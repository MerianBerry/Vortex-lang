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

//LLVM includes
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#if defined(NDEBUG)
#define NPRINT
#endif

namespace c = std::chrono;

/*
struct leaf
{
    char* name = NULL;
    size_t size = 0;
    void* ptr = nullptr;
    leaf* last = nullptr;
    leaf* next = nullptr;
};


struct binleaf
{
    char* name = NULL;
    size_t size = 0;
    void* ptr = nullptr;
    binleaf* right = nullptr;
    binleaf* left = nullptr;
    binleaf* root = nullptr;
};

class BinaryTree
{
    private:
        uint32_t count = 0;
        binleaf* first = nullptr;
        binleaf* last = nullptr;
        bool destroyed = false;
    public:
    template<class T>
    binleaf* add(const char* name, T value)
    {
        destroyed = false;

        binleaf* leaflet = (binleaf*)malloc(sizeof(binleaf));
        leaflet->size = sizeof(value);
        
        leaflet->name = (char*)name;
        leaflet->ptr = (void*)malloc(leaflet->size);
        memcpy(leaflet->ptr, &value, leaflet->size);

        if (first == nullptr)
        {
            first = leaflet;
        } else
        {
            binleaf* itr = first;
            while(true)
            {
                if (strcmp(itr->name, leaflet->name) < 0)
                {
                    //printf("Right\n");
                    if (itr->right == nullptr)
                    {
                        //printf("Push on %p right\n", itr);
                        itr->right = leaflet;
                        break;
                    }

                    //printf("Move to %p right\n", itr->right);
                    itr = itr->right;
                    continue;
                } else
                {
                    //printf("left\n");
                    if (itr->right == nullptr)
                    {
                        //printf("Push on %p left\n", itr);
                        itr->left = leaflet;
                        break;
                    }

                    //printf("Move to %p left\n", itr->left);
                    itr = itr->left;
                    continue;
                }
            }
        }
        last = leaflet;

        return leaflet;
    }

    binleaf* begin()
    {
        return first;
    }
}; 

class LeafArray
{
private:
    uint32_t count = 0;
    leaf* first = nullptr;
    leaf* last = nullptr;
    bool destroyed = false;
public:
    template<class T>
    leaf* add(const char* name, T value)
    {
        destroyed = false;
        leaf* l = (leaf*)malloc(sizeof(leaf));
        
        l->size = sizeof(value);
        //std::unique_ptr<T> ptr = std::make_unique<T>(value);
        
        //printf("j = %s\n", *(T*)ptr.get());
        //l->ptr = ptr.get();
        l->ptr = (void*)malloc(l->size);
        memcpy(l->ptr, &value, l->size);

        //l->name = (char*)calloc(strlen(name)+1, 1);
        //strcpy(l->name, name);
        l->name = (char*)name;
        l->last = last;
        
        //printf("current pointer: %p\n", l);        

        if (last != nullptr)
        {
            last->next = l;
            //printf("pointer: %p\n", last->next);
            l->last = last;
        }
        last = l;
        if (count == 0)
        {
            first = l;
        }
        ++count;
        return l;
    }
    uint32_t size()
    {
        return count;
    }

    leaf* get(const char* name)
    {
        leaf* itr = first;
        while(true)
        {
            //printf("next pointer: %p\n", itr->next);
            if (strcmp(itr->name, name) == 0)
            {
                return itr;
            }
            if (itr == nullptr || itr->next == nullptr)
            {
                return nullptr;
            }
            itr = itr->next;
        }
    }
    leaf* destroy(const char* name)
    {
        auto leaflet = get(name);
        if (leaflet != nullptr)
        {
            auto leaflast = leaflet->last;
            if (leaflast != nullptr)
            {
                leaflet->last->next = leaflet->next;
                leaflet->next->last = leaflast;
            }
            free(leaflet->ptr);
            free(leaflet);
            count--;
            return leaflast;
        }
        return nullptr;
    }

    LeafArray()
    {

    }
    ~LeafArray()
    {
        if (!destroyed)
        {
            //printf("Leaf storage about to be destroyed\n");
            leaf* itr = first;
            while(true)
            {
                //printf("itr pointer: %p\n", itr);
                if (itr == nullptr || itr->next == nullptr)
                {
                    break;
                }
                free(itr->ptr);
                itr = itr->next;
                if (itr->last != nullptr)
                {
                    free(itr->last);
                }
            }
        }
        destroyed = true;
        count = 0;
    }
};*/

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
static long double numval = 0.0;

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
};

class NumberExprAST : public ExprAST
{
    long double Val = 0.0;

    public:
    NumberExprAST(long double Val) : Val(Val) {}
};

class VariableExprAST : public ExprAST
{
    char Name[512] = "";

    public:
    VariableExprAST(const char* name)
    {
        memcpy(Name, name, std::min(strlen(name), sizeof(Name)));
    }
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

};

class PrototypeAST
{
    char Name[512] = "";
    std::vector<const char*> Args;

    public:
    PrototypeAST(const char* name, std::vector<const char*> args)
    {
        memcpy(Name, name, std::min(strlen(name), sizeof(Name)));
        Args = std::move(args);
    }
};

class FunctionAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

    public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
        std::unique_ptr<ExprAST> Body) :
            Proto(std::move(Proto)), Body(std::move(Body)) {}
};

//-----------------//
//-- PROCESSING --//
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

static std::unique_ptr<ExprAST> ParseNumberExpr()
{
    auto res = std::make_unique<NumberExprAST>(numval);
    getNextToken();
    return std::move(res);
}

static std::unique_ptr<ExprAST> ParseParenExpr()
{
    getNextToken();

    auto V = ParseExpression();
    if (!V)
    {
        return nullptr;
    }

    if (Curtok != ')')
        return LogError("expected ')'");
    getNextToken();
    return V;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    char idname[strlen(identstr)+1];
    memcpy(idname, identstr, strlen(identstr));

    getNextToken();

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
        getNextToken();

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
        printf("h > %i\n", Curtok);
        return LogErrorP("expected function name in prototype");
    }
    

    char fnname[strlen(identstr)+1];
    memcpy(fnname, identstr, strlen(identstr));
    getNextToken();

    if (Curtok != '(')
    {
        return LogErrorP("expected '(' in prototype function");
    }

    // Read the list of arg names
    std::vector<const char*> Argnames;
    while(getNextToken() == tok_ident)
    {
        //printf("Argname: %s\n", identstr);
        Argnames.push_back(identstr);
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
        auto P = std::make_unique<PrototypeAST>("", std::vector<const char*>());
        return std::make_unique<FunctionAST>(std::move(P), std::move(E));
    }
    return nullptr;
}

static void HandleDef()
{
    if (ParseDefinition())
    {
        #if !defined(NPRINT)
        fprintf(stderr, "Parsed a function definition\n");
        #endif
    } else // error recovery
        getNextToken();
}

static void HandleExt()
{
    if (ParseExtern())
    {
        #if !defined(NPRINT)
        fprintf(stderr, "Parsed a function definition\n");
        #endif
    } else // error recovery
        getNextToken();
}

static void HandleTopLevelExpr()
{
    if (ParseExtern())
    {
        #if !defined(NPRINT)
        fprintf(stderr, "Parsed a top-level expression\n");
        #endif
    } else // error recovery
        getNextToken();
}

//-----------------//
//-- DRIVER --//
//-----------------//
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