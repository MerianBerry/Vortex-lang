//C includes
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>

//CXX includes
#include <memory>
#include <chrono>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>

//LLVM includes
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "JIT.h"

#if defined(NDEBUG)
#define NPRINT
#endif

namespace c = std::chrono;

template<typename ... Args>
inline std::string stringf( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    auto buf = std::make_unique<char[]>( size );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

static unsigned int g_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
inline long double fastrand() { 
  g_seed = (214013*g_seed+2531011); 
  return (long double)((g_seed>>16)&0x7FFF) / (long double)0x7FFF; 
} 

//-----------------//
//-- LEXER --//
//-----------------//

static FILE* file = nullptr;
enum toks
{
    tok_func = -100,

    tok_extern,

    tok_ident,
    tok_number
};

const uint32_t keysize = 2;
const std::pair<const char*, int> keywords[] = {
    {"function", tok_func},
    {"extern", tok_extern}
};

static uint64_t row = 1;
static uint64_t col = 1;
static int lastchar = ' ';
static char identstr[1024] = "";
static double numval = 0.0;

int getnext()
{
    auto ch = getc(file);
    ++col;
    if (ch == '\n' || ch == '\r')
    {
        col = 1;
        ++row;
    }
    return ch;
}

static int ParseNonSpace()
{
    while (isspace(lastchar))
    {
        lastchar = getnext();
    }

    if (lastchar != EOF)
    {
        memset(identstr, 0, sizeof(identstr));
        int type = -1;

        do
        {
            if (isalpha(lastchar) && type == -1)
            {
                type = 0;
            } else if ((isdigit(lastchar) || lastchar == '.') && type == -1)
            {
                type = 1;
            } else if (type == -1)
            {
                type = 2;
            }

            if (type == 0 && !isalnum(lastchar))
                break;
            else if (type == 1 && !(isdigit(lastchar) || lastchar == '.'))
                break;
            else if (type == 2 && (isalnum(lastchar)))
                break;

            const char ch[1] = {(char)lastchar};
            strncat(identstr, ch, 1);
        } while (!isspace((lastchar = getnext())) && lastchar != EOF);
        
        #if !defined(NPRINT)
        
        #endif
        return type;
    }

    return -1;
}

static int gettokd()
{
    const int type = ParseNonSpace();

    switch (type)
    {
    case 0:
        //-- KEYWORDS --//
        for (uint32_t i = 0; i < keysize; ++i)
        {
            if (strcasecmp(keywords[i].first, identstr) == 0)
            {
                return keywords[i].second;
            }
        }
        return tok_ident;
        break;
    
    case 1:
        numval = strtold(identstr, nullptr);
        return tok_number;
        break;
    
    case 2:
        if (strcmp(identstr, "//")==0)
        {
            do
            {
                lastchar = getnext();
            } while (lastchar != EOF && lastchar != '\n' && lastchar != '\r');
            if (lastchar != EOF)
                return gettokd();
        }
        break;

    case -1:
        return EOF;
        break;
    default:
        break;
    }
    
    if (lastchar == EOF)
        return EOF;

    int thischar = lastchar;
    lastchar = getnext();
    return thischar;
}

static int gettok()
{
    while (isspace(lastchar))
        lastchar = getnext();

    if (isalpha(lastchar))
    {
        memset(identstr, 0, sizeof(identstr));
        while (isalnum(lastchar))
        {
            const char ch[1] = {(char)lastchar};
            strncat(identstr, ch, 1);
            lastchar = getnext();
        }
        //printf("Ident str: \"%s\"\n", identstr);

        //-- KEYWORDS --//
        for (uint32_t i = 0; i < keysize; ++i)
        {
            if (strcmp(keywords[i].first, identstr) == 0)
            {
                return keywords[i].second;
            }
        }
        
        return tok_ident;
    }

    if (isdigit(lastchar) || lastchar == '.')
    {
        char numstr[1024] = "";
        do
        {
            //printf("lastchar = %i\n", lastchar);
            const char ch[1] = {(char)lastchar};
            strncat(numstr, ch, 1);
            lastchar = getnext();
        } while (isdigit(lastchar) || lastchar == '.');
        numval = strtold(numstr, nullptr);
        return tok_number;
    }

    if (lastchar == '#')
    {
        do
        {
            lastchar = getnext();
        } while (lastchar != EOF && lastchar != '\n' && lastchar != '\r');

        if (lastchar != EOF)
            return gettok();
    }

    if (lastchar == EOF)
        return EOF;

    int thischar = lastchar;
    lastchar = getnext();
    return thischar;
}


//-----------------//
//-- BIG RED TREE --//
//-----------------//

class ExprAST
{
    public:
        virtual ~ExprAST() = default;
        virtual llvm::Value *codegen() = 0;
};

class StringExprAST : public ExprAST
{
    std::string Val;

    public:
    StringExprAST(std::string Val) : Val(Val) {}
    llvm::Value *codegen() override;
};

class NumberExprAST : public ExprAST
{
    double Val;

    public:
    NumberExprAST(double Val) : Val(Val) {}
    llvm::Value *codegen() override;
};

class VariableExprAST : public ExprAST
{
    char Name[512] = "";

    public:
    VariableExprAST(const char* name)
    {
        memcpy(Name, name, std::min(strlen(name), sizeof(Name)));
    }
    llvm::Value *codegen() override;
};

class BinaryExprAST : public ExprAST
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

    public:
    BinaryExprAST(char op,
        std::unique_ptr<ExprAST> LHS,
        std::unique_ptr<ExprAST> RHS) :
            Op(op),
            LHS(std::move(LHS)),
            RHS(std::move(RHS)) {}
    llvm::Value *codegen() override;
};

class CallExprAST : public ExprAST
{
    char Callee[512] = "";
    std::vector<std::unique_ptr<ExprAST>> Args;

    public:
    CallExprAST(const char* callee,
        std::vector<std::unique_ptr<ExprAST>> args)
    {
        memcpy(Callee, callee, std::min(strlen(callee), sizeof(Callee)));
        Args = std::move(args);
    }
    llvm::Value *codegen() override;
};

class PrototypeAST
{
    char Name[512] = "";
    std::vector<std::string> Args;

    public:
    PrototypeAST(const char* name, std::vector<std::string> args)
    {
        memcpy(Name, name, std::min(strlen(name), sizeof(Name)));
        Args = args;
    }
    llvm::Function *codegen();
    const char* getName() const {return Name;}
};

class FunctionAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

    public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
        std::unique_ptr<ExprAST> Body) :
            Proto(std::move(Proto)), Body(std::move(Body)) {}
    llvm::Function *codegen();
};

//-----------------//
//-- PARSING --//
//-----------------//

static std::unique_ptr<ExprAST> ParseExpression();

static int Curtok;
static int getNextToken()
{
    return Curtok = gettok();
}

std::unique_ptr<ExprAST> LogError(const char* str)
{
    fprintf(stderr, "Error at [%zu, %zu]: %s\n",row, col, str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char* str)
{
    LogError(str);
    return nullptr;
}
llvm::Value *LogErrorV(const char *str)
{
    LogError(str);
    return nullptr;
}

void LogStatus(const char* str)
{
    #if !defined(NPRINT)
    fprintf(stderr, "STATUS [%lu, %lu]: %s", row, col, str);
    #endif
}

static std::unique_ptr<ExprAST> ParseNumberExpr()
{
    //printf("Numval = %f\n", numval);
    auto res = std::make_unique<NumberExprAST>(numval);
    getNextToken();
    return std::move(res);
}

static std::unique_ptr<ExprAST> ParseParenExpr()
{
    getNextToken(); // eat '('

    auto V = ParseExpression();
    if (!V)
    {
        return nullptr;
    }

    if (Curtok != ')')
        return LogError("expected ')'");
    getNextToken(); // eat ')'
    return V;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    char idname[strlen(identstr)];
    strcpy(idname, identstr);
    //memcpy(idname, identstr, strlen(identstr));
    getNextToken(); // eat identifier

    if (Curtok != '(') // simple var ref
        return std::make_unique<VariableExprAST>((char*)idname);
    
    // Call.
    getNextToken(); // Eat (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (Curtok != ')')
    {
        while (true)
        {
            if (auto Arg = ParseExpression())
            {
                Args.push_back(std::move(Arg));
            } else 
                return nullptr;
            
            if (Curtok == ')')
                break;

            if (Curtok != ',') // if ',' is expected
                return LogError("expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    getNextToken(); // Eat the ')'

    return std::make_unique<CallExprAST>((char*)idname, std::move(Args));
}

static std::unique_ptr<ExprAST> ParsePrimary()
{
    switch (Curtok)
    {
        default:
            return LogError("unknown token when expecting an expression");
        case tok_ident:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}

static std::map<char, int> Binopmap;

static int GetTokPrecedence()
{
    if (!isascii(Curtok))
    {
        return -1;
    }

    int TokPrec = Binopmap[Curtok];
    if (TokPrec <= 0) return -1;

    return TokPrec;
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS)
{
    // If this is a binop, find its precedence
    while(true)
    {
        int TokPrec = GetTokPrecedence();

        if (TokPrec < ExprPrec)
            return LHS;

        // Now we know it is a valid binop
        int BinOp = Curtok;
        getNextToken(); // eat binop
        //fprintf(stderr, "op = %c\n", BinOp);
        //printf("RHS = \"%s\" or \"%f\"\n", identstr, numval);

        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        // If binop binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec)
        {
            RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // Merge LHS/RHS
        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

static std::unique_ptr<ExprAST> ParseExpression()
{
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;
    
    return ParseBinOpRHS(0, std::move(LHS));
}

static std::unique_ptr<PrototypeAST> ParsePrototype()
{
    //getNextToken(); // Consume function name

    //LogErrorP("Not an error");
    if (Curtok != tok_ident)
    {
        //printf("h > %i\n", Curtok);
        return LogErrorP("expected function name in prototype");
    }
    

    char fnname[strlen(identstr)+1];
    strcpy(fnname, identstr);
    //memcpy(fnname, identstr, strlen(identstr));
    getNextToken();

    if (Curtok != '(')
    {
        return LogErrorP("expected '(' in prototype function");
    }

    // Read the list of arg names
    std::vector<std::string> Argnames;
    getNextToken();
    while(Curtok == tok_ident)
    {
        //printf("Token = %i\n", Curtok);
        //printf("Argname: %s\n", identstr);
        Argnames.push_back(identstr);

        getNextToken();
        if (Curtok != ',' && Curtok != ')')
        {
            return LogErrorP("expected ',' in arg list or ')' in prototype function");
        }
        if (Curtok == ')')
            break;
        getNextToken();
    }
    if (Curtok != ')')
        return LogErrorP("expected ')' in prototype function");

    // success
    getNextToken(); // eat ')'

    return std::make_unique<PrototypeAST>((char*)fnname, std::move(Argnames));
}

static std::unique_ptr<FunctionAST> ParseDefinition()
{
    getNextToken(); // eat "function"
    auto P = ParsePrototype();
    if (!P) return nullptr;

    
    if (auto E = ParseExpression())
    {
        return std::make_unique<FunctionAST>(std::move(P), std::move(E));
    }
    return nullptr;
}

static std::unique_ptr<PrototypeAST> ParseExtern()
{
    getNextToken(); // eat "extern"
    return ParsePrototype();
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
    if (auto E = ParseExpression())
    {
        // Make an anonymous proto
        auto P = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(P), std::move(E));
    }
    return nullptr;
}

//-----------------//
//-- CODEGEN --//
//-----------------//

using namespace llvm; using namespace llvm::orc;
using std::unique_ptr; using std::make_unique;

static unique_ptr<LLVMContext> TheContext;
static unique_ptr<IRBuilder<>> Builder;
static unique_ptr<Module> TheModule;
static unique_ptr<legacy::FunctionPassManager> TheFPM;
static unique_ptr<JIT> TheJIT;
struct cmp_str
{
    bool operator()(char *a, char *b) const
    {
        return strcmp(a, b) < 0;
    }
};
static std::map<char*, Value*, cmp_str> NamedValues;

Value *StringExprAST::codegen()
{
    return ConstantDataArray::getString(*TheContext, Val);
}

Value *NumberExprAST::codegen()
{
    return ConstantFP::get(*TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen()
{
    // Look this vairable up in the function
    Value *V = NamedValues[Name];
    if (!V)
    {
        LogErrorV(stringf("unknown variable name: \"%s\"", Name).c_str());
    }
    return V;
}

Value *BinaryExprAST::codegen()
{
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R)
        return nullptr;
    
    //return std::get<2>(Binop)
    
    switch (Op)
    {
        case '+':
            return Builder->CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder->CreateFSub(L, R, "subtmp");
        case '*':
            return Builder->CreateFMul(L, R, "multump");
        case '/':
            return Builder->CreateFDiv(L, R, "divtmp");
        case '<':
            L = Builder->CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
        default:
            return LogErrorV("invalid binary operator");
    }
}

Value *CallExprAST::codegen()
{
    // Look up the name in the global module table
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
    {
        return LogErrorV("unknown function referenced");
    }

    // if arg mismatch error
    if (CalleeF->arg_size() != Args.size())
    {
        return LogErrorV("argument count mismatch");
    }

    std::vector<Value*> ArgsV;
    for (uint32_t i = 0, e = Args.size(); i != e; ++i)
    {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::codegen()
{
    // Make the function type: double(double, double) etc
    std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(*TheContext));

    FunctionType *FT =
        FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);

    Function *F =
        Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    // Set names for all arguments
    uint32_t ind = 0;
    for (auto &Arg : F->args())
    {
        //fprintf(stderr, "Setting function argument to \"%s\"\n", Args[ind].c_str());
        Arg.setName(Args[ind]);
        ++ind;
    }

    return F;
}

Function *FunctionAST::codegen()
{
    // First, check for an existing funciton from a previous 'extern' declaration
    Function *TheFunction = TheModule->getFunction(Proto->getName());
    
    if (!TheFunction)
    {
        TheFunction = Proto->codegen();
    }

    if (!TheFunction)
        return nullptr;

    if (!TheFunction->empty())
    {
        return (Function*)LogErrorV("function cannot be redefined");
    }

    // Create a new basic block to start insertion info
    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map
    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
    {
        NamedValues[(char*)Arg.getName().data()] = &Arg;
        //fprintf(stderr, "Recorded function var %s, %p\n", (char*)Arg.getName().data(), NamedValues[(char*)Arg.getName().data()]);
    }

    if (Value *RetVal = Body->codegen())
    {
        // Finish off the function
        Builder->CreateRet(RetVal);

        // Validate the generated code, checking for consistency
        verifyFunction(*TheFunction);

        TheFPM->run(*TheFunction);

        return TheFunction;
    }

    // Error reading body, remove function
    TheFunction->eraseFromParent();
    return nullptr;
}

//-----------------//
//-- DRIVER --//
//-----------------//

static void InitializeModuleAndPassManager()
{
    // Open a new context, JIT, and module
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("Vortex JIT", *TheContext);
    TheModule->setDataLayout(TheJIT->getDataLayout());

    // Create a new builder for the module
    Builder = std::make_unique<IRBuilder<>>(*TheContext);

    TheFPM = make_unique<legacy::FunctionPassManager>(TheModule.get());

    // Do simple "peephole" optimizations and bit-twiddling opts
    TheFPM->add(createInstructionCombiningPass());
    // Reassociate expressions
    TheFPM->add(createReassociatePass());
    // Eliminate common SubExpressions
    TheFPM->add(createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc)
    TheFPM->add(createCFGSimplificationPass());


    TheFPM->doInitialization();
}

static ExitOnError ExitOnErr;

static void HandleDef()
{
    if (auto FnAST = ParseDefinition())
    {
        if (auto *FnIR = FnAST->codegen())
        {
            LogStatus("Read a function definition:\n");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else // error recovery
        getNextToken();
}

static void HandleExt()
{
    if (auto ProtoAST = ParseExtern())
    {
        if (auto *FnIR = ProtoAST->codegen())
        {
            LogStatus("Read extern:\n");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else // error recovery
        getNextToken();
}

static void HandleTopLevelExpr()
{
    if (auto FnAST = ParseTopLevelExpr())
    {
        //LogStatus("Parsed function\n");
        //auto start = c::high_resolution_clock::now();
        //InitializeModuleAndPassManager();
        //auto end = c::high_resolution_clock::now();
        //printf("Duration: %.2fus\n", c::duration<float, c::microseconds::period>(end-start).count());
        if (FnAST->codegen())
        {
            // Craete a Resource Tracker to track JIT'd memory allocated to our
            // anonymous expression -- that way we can free it after executing
            auto RT = TheJIT->getMainJITDylib().createResourceTracker();

            auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
            //printf("PTR = %p\n", TSM.getModuleUnlocked());
            ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
            InitializeModuleAndPassManager();

            // Search the JIT for the __anon_expr symbol
            auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));
            if (!ExprSymbol)
                LogError("function \"__anon_expr\" not found\n");

            // Get the symbol's address and cast it to the right type
            // (takes no args, returns a double) so we can call it as a native function
            double (*FP)() = (double (*)())(intptr_t)ExprSymbol.getAddress();
            LogStatus(stringf("Evaluated to %.3f\n", FP()).c_str());

            // Delete anonymous expression module from the JIT
            ExitOnErr(RT->remove());
        }
    } else // error recovery
        getNextToken();
}

static void VTEXMain()
{
    getNextToken();
    while(true)
    {
        switch (Curtok)
        {
            case EOF:
                return;
            case ';':
                getNextToken();
                break;
            case tok_func:
                //fprintf(stderr, "Processing definition\n");
                HandleDef();
                break;
            case tok_extern:
                HandleExt();
                break;
            default:
                HandleTopLevelExpr();
                break;
        }
    }
}

std::tuple<char, int, Value*(*)(Value*, Value*)> h =
{'+', 10, [](Value *L, Value *R) -> Value*
{
    return Builder->CreateFAdd(L, R, "faddtmp");
}};

static std::vector<std::pair<char, int>> binopvec = 
{
    {'<', 10},
    {'>', 10},
    {'+', 20},
    {'-', 20},
    {'*', 40},
    {'/', 40}
};

static bool VTEXSetup(const char* fileName)
{   
    FILE* fi = fopen(fileName, "r");
    if (!fi)
    {
        return false;
    }
    file = fi;
    for (const auto& [name, prec] : binopvec)
    {
        Binopmap[name] = prec;
    }
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    TheJIT = ExitOnErr(JIT::Create());
    InitializeModuleAndPassManager();
    return true;
}

static void VTEXCleanup()
{
    fclose(file);
}

int main(int argc, char* argv[])
{
    if (VTEXSetup("example.vtex"))
    {
        auto start = c::high_resolution_clock::now();
        VTEXMain();
        auto end = c::high_resolution_clock::now();
        fprintf(stderr, "Parse time: %.2Lfus\n", c::duration<long double, c::microseconds::period>(end-start).count());

        VTEXCleanup();
    }
}