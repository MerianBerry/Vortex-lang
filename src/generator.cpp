// Vortex headers
#include "types.h"
#include "operators.h"
#include "tokens.h"
#include "keywords.h"
#include "value.h"
#include "vstring.h"

// C headers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>
#include <ctime>

// CXX headers
#include <chrono> // For duration diagnostics
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <iomanip>

namespace c = std::chrono;

bool verbose = true;

unsigned int g_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
inline long double fastrand() { 
  g_seed = (214013*g_seed+2531011); 
  return (long double)((g_seed>>16)&0x7FFF) / (long double)0x7FFF; 
} 

FILE* __logfile = NULL;
template<typename ... Args>
void* Logf(std::string fmt, Args ... args)
{
    if (__logfile == nullptr)
    {
        __logfile = fopen("log.txt", "w");
    }
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    fprintf(__logfile, "[%s]: %s\n", oss.str().c_str(), stringf(fmt.c_str(), args...).c_str());
    #if !defined(NPRINT)
    fprintf(stderr, "[%s]: %s\n", oss.str().c_str(), stringf(fmt.c_str(), args...).c_str());
    #endif

    return nullptr;
}

#pragma region "Lexer"

std::string file;
size_t fiindex = 0;
int lastchar = ' ';
int curtok = ' ';
std::string identstr = "";
std::string numstr = "";
std::string stringstr = "";
long long column = -1;
size_t line = 1;

void newlevel(const std::string &str)
{
    //printf("String size = %lu\n", str.size());
    file = str;
    file+= "  ";
}

static int getnext()
{
    if (fiindex >= file.size()-1)
    {
        //printf("End at index %lu\n", fiindex);
        return EOF;
    }
    char c = file[fiindex];
    ++fiindex;
    //printf("strchar = '%c'\n", (char)c);
    
    ++column;
    if (lastchar == '\n' || lastchar == '\r')
    {
        column = 1;
        ++line;
    }
    return c;
}

static int gettok()
{
    while(isspace(lastchar))
        lastchar = getnext();
    
    if (isalpha(lastchar))
    {
        identstr = "";
        
        do
        {
            identstr += lastchar;
        } while (isalnum((lastchar = getnext())));

        for (const auto& [name, tok] : vtex::keys)
        {
            if (strcasecmp(name, identstr.c_str()) == 0)
            {
                return tok;
            }
        }

        return tok_ident;
    }

    if (isdigit(lastchar))
    {
        numstr = "";

        do
        {
            numstr += lastchar;
        } while (isdigit((lastchar = getnext())) || lastchar == '.');

        return tok_number;
    }

    if (lastchar == '\"')
        return tok_string;

    if (lastchar == EOF)
        return EOF;

    auto thischar = lastchar;
    lastchar = getnext();
    return thischar;
}

static int getnexttoken()
{
    return (curtok = gettok());
}

#pragma endregion // End lexer region


#pragma region "Abstract Syntax Tree"

class Expr
{
    public:
        virtual ~Expr() = default;
        virtual std::string tostring() {return "";};
        virtual int codegen() {return -2;}
};

class StringExpr : public Expr
{
    vtex::String Val;
    public:
        StringExpr(vtex::String Val) : Val(Val) {}
        std::string tostring() override
        {
            return stringf("\"%s\"", Val.tostring().c_str());
        }
        int codegen() override;
};

class NumberExpr : public Expr
{
    vtex::LFloat Val;
    public:
        NumberExpr(vtex::LFloat Val) : Val(Val) {}
        std::string tostring() override
        {
            return Val.tostring();
        }
        int codegen() override;
};

class VariableExpr : public Expr
{
    std::string Name = "";
    bool isnan = true;
    public:
        VariableExpr(std::string Name) : Name(Name) {}
        std::string tostring()
        {
            return Name;
        }
        int codegen() override;
};

class ValueExpr : public Expr
{
    std::unique_ptr<vtex::Value> Val;
    public:
        ValueExpr(std::unique_ptr<vtex::Value> Val) : Val(std::move(Val)) {}
        std::string tostring()
        {
            if (!Val)
                return "null";
            return Val->val->tostring();
        }
        int codegen() override;
};

class BinopExpr : public Expr
{
    std::string Op = "__null";
    std::unique_ptr<Expr> LHS, RHS;
    public:
        BinopExpr(std::string Op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS) :
            Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    std::string tostring() override
    {
        return stringf("%s %s %s", LHS->tostring().c_str(), Op.c_str(), RHS->tostring().c_str());
    }
};

class ProtoExpr : public Expr
{
    std::string Name = "__anon_function";
    std::vector<std::string> Argnames = {"__void"};
    public:
        ProtoExpr() {}
        ProtoExpr(std::string name, std::vector<std::string> args) : Name(name)
        {
            if (args.size() != 0)
            {
                Argnames = args;
            }
        }
    std::string tostring() override
    {
        std::string str = Name + " : ";
        int a = 0;
        for (const auto& name : Argnames)
        {
            if (a==0)
            str+=name;
            else
            str+= ", " +name;
            a++;
        }
        return str;
    }
};

class FunctionExpr : public Expr
{
    std::unique_ptr<ProtoExpr> Proto;
    std::vector<std::unique_ptr<Expr>> Body;
    public:
        FunctionExpr(std::unique_ptr<ProtoExpr> proto, std::vector<std::unique_ptr<Expr>>&& body) 
        : Proto(std::move(proto))
        {
            for (auto& h : body)
            {
                Body.push_back(std::move(h));
            }
        }
    std::string tostring() override
    {
        std::string str = Proto->tostring()+"\n{";
        for (auto& E : Body)
        {
            //auto h = E.tostring();
            //printf("'%s'\n", E.=->tostring().c_str());
            str+="\n\t"+E->tostring()+";";
        }
        str+="\n}";
        return str;
    }
};

#pragma endregion // End AST region


#pragma region "Parser"

 std::unordered_map<std::string, std::unique_ptr<VariableExpr>> usrvarmap;
 std::unordered_map<std::string, int> binopmap;

 std::unique_ptr<Expr> LogError(const char* str)
{
    //if (verbose)
    fprintf(stderr, "ERROR [Ln %zu, Col %lld]: %s\n", line, column, str);
    return nullptr;
}
 std::unique_ptr<Expr> LogStatus(const char* str)
{
    if (verbose)
    fprintf(stderr, "Status [Ln %zu, Col %lld]: %s\n", line, column, str);
    return nullptr;
}
 void* LogNote(const char* str)
{
    //if (verbose)
    fprintf(stderr, "Note [Ln %zu, Col %lld]: %s\n", line, column, str);
    return nullptr;
}

 std::unique_ptr<vtex::Value> ParseString()
{
    stringstr = "";
    while((curtok = getnext()) != '\"' && curtok != EOF)
    {
        stringstr += curtok;
    }
    if (curtok == EOF)
    {
        LogError("string literall extends to EOF");
        return nullptr;
    }

    lastchar = getnexttoken(); // Eat ending '"' and restore the lexer
    return std::make_unique<vtex::Value>(std::make_unique<vtex::String>(stringstr));
}

 std::unique_ptr<Expr> ParseDefinition()
{
    getnexttoken(); // Eat "new"

    if (curtok != tok_ident)
        return LogError("expected an identifier after \"new\"");
    
    usrvarmap[identstr.c_str()] = std::make_unique<VariableExpr>(identstr.c_str());
    LogStatus(stringf("New variable \"%s\"", identstr.c_str()).c_str());

    return std::make_unique<VariableExpr>(identstr.c_str());
}

 std::unique_ptr<ProtoExpr> ParsePrototype()
{
    getnexttoken(); // Eat "function"
    //LogStatus("Found function specifier");
    if (curtok == tok_ident)
    {
        LogStatus("Found function variable");
        std::string name = identstr;
        getnexttoken();
        if (curtok != '(')
        {
            LogError("expected '(' after function specifier");
            return nullptr;
        }
        getnexttoken();
        std::vector<std::string> argnames = {};
        while(curtok != ')')
        {
            if (curtok != tok_ident)
            {
                LogError("expected identifier in prototype argument definitions");
                return nullptr;
            }
            LogStatus(stringf("Function parameter: \"%s\"", identstr.c_str()).c_str());
            argnames.push_back(identstr);
            getnexttoken();

            
            if (curtok != ',' && curtok == ')')
            {
                break;
            } else if (curtok != ',' && curtok != ')')
            {
                LogError("expected ',' or ')' in prototype argument definitions");
                return nullptr;
            }
            getnexttoken();
        }
        return std::make_unique<ProtoExpr>(name, argnames);
    } else
    {
        LogStatus("Found first class function");
        if (curtok != '(')
        {
            LogError("expected '(' after function specifier");
            return nullptr;
        }
        getnexttoken();
        std::vector<std::string> argnames = {};
        while(curtok != ')')
        {
            if (curtok != tok_ident)
            {
                LogError("expected identifier in prototype argument definitions");
                return nullptr;
            }
            LogStatus(stringf("Function parameter: \"%s\"", identstr.c_str()).c_str());
            argnames.push_back(identstr);
            getnexttoken();

            
            if (curtok != ',' && curtok == ')')
            {
                break;
            } else if (curtok != ',' && curtok != ')')
            {
                LogError("expected ',' or ')' in prototype argument definitions");
                return nullptr;
            }
            getnexttoken();
        }
        return std::make_unique<ProtoExpr>("__anon_function", argnames);
    }
}

 std::vector<std::unique_ptr<Expr>> ParseScope();

 std::unique_ptr<Expr> ParseFunction()
{
    std::unique_ptr<ProtoExpr> P = ParsePrototype();
    std::vector<std::unique_ptr<Expr>> B = {};

    getnexttoken();
    if (curtok != '{' && !!P)
    {
        LogNote("Function has no scope");
        return std::make_unique<FunctionExpr>(std::move(P), std::vector<std::unique_ptr<Expr>>(0));
    } else if (!!P)
    {
        B = ParseScope();
        if (B.size() != 0)
        {
            return std::make_unique<FunctionExpr>(std::move(P), std::move(B));
        }
        LogError("Scope body is nullptr, proceeding with proto only");
        return std::make_unique<FunctionExpr>(std::move(P), std::vector<std::unique_ptr<Expr>>(0));
    }

    return LogError("Function prototype is invalid, cannot proceed with function creation");
}

 std::unique_ptr<Expr> ParsePrimary()
{
    switch(curtok)
    {
        case tok_number:
            return std::make_unique<ValueExpr>(std::make_unique<vtex::Value>(std::make_unique<vtex::LFloat>(strtold(numstr.c_str(), nullptr))));
        case tok_string:
            return std::make_unique<ValueExpr>(ParseString());
        case tok_ident:
            if (usrvarmap.find(identstr) != usrvarmap.end())
            {
                LogStatus(stringf("Variable read operation on \"%s\"", identstr.c_str()).c_str());
                return std::make_unique<VariableExpr>(identstr);
            } else
            {
                return LogError(stringf("Unknown identity \"%s\" in expression", identstr.c_str()).c_str());
            }
        case tok_func:
            return ParseFunction();
        default:
            return LogError(stringf("unknown token %i in expression", curtok).c_str());
    }
}

 int GetBinopPrec()
{
    std::string str = static_cast<std::string>("")+(char)curtok;
    auto itr = binopmap.find(str);
    if (itr == binopmap.end())
        return -1;
    return itr->second;
}

 std::unique_ptr<Expr> ParseRHS(int Expected, std::unique_ptr<Expr> LHS)
{
    while(true)
    {
        int Prec = GetBinopPrec();
        if (Prec < Expected)
        {
            return LHS;
        }

        auto op = static_cast<std::string>("")+(char)curtok;

        getnexttoken(); // Eat binop
        auto RHS = ParsePrimary();
        if (!RHS)
        {
            return LogError("Right hand symbol (RHS) is null");
        }

        getnexttoken(); // Eat the primary

        // If the next binop has more precedence, give it RHS with a higher expected precedence
        int Next = GetBinopPrec();
        if (Prec < Next)
        {
            RHS = ParseRHS(Prec+1, std::move(RHS));
            if (!RHS)
            {
                return LogError("Right hand symbol (RHS) is null");
            } else
            {
                LogStatus(stringf("Higher level RHS = %s", RHS->tostring().c_str()).c_str());
            }
        }

        // Git branch merge
        LHS = std::make_unique<BinopExpr>(op, std::move(LHS), std::move(RHS));
    }
}

 std::unique_ptr<Expr> ParseExpression()
{
    std::unique_ptr<Expr> LHS;
    std::unique_ptr<Expr> RHS;

    LHS = ParsePrimary();

    if (!LHS)
    {
        LogNote("Left hand symbol (LHS) is null");
        return nullptr;
    }
    getnexttoken();

    RHS = ParseRHS(0, std::move(LHS));
    if (!!RHS)
    {
        LogStatus(stringf("RHS = %s", RHS->tostring().c_str()).c_str());
    }
    return RHS;
}

 std::unique_ptr<Expr> ParseSet()
{
    getnexttoken(); // Eat "="

    auto LHS = ParseExpression();
    if (LHS != nullptr)
    {
        LogStatus(stringf("LHS = %s", LHS->tostring().c_str()).c_str());
    }
    
    return nullptr;
}

 std::unique_ptr<Expr> ParseIdentity()
{
    auto lident = identstr;

    getnexttoken(); // Eat identity

    switch(curtok)
    {
        /*
        case '=':
            if (usrvarmap.find(identstr) == usrvarmap.end())
                LogError("variable name does not exist");
            LogStatus(stringf("Variable set operation on \"%s\"", lident.c_str()).c_str());
            ParseSet();
            break;*/
        default:
            return nullptr;//LogStatus("nothing to parse for identity");
    }

    return nullptr;
}

 std::unique_ptr<Expr> ParseAny()
{
    switch(curtok)
    {
        case EOF:
            return nullptr;
        case tok_new:
            return ParseDefinition();
            //break;
        case tok_func:
        {
            auto F = ParseFunction();
            if (!!F)
                LogStatus(stringf("Function: %s", F->tostring().c_str()).c_str());
            return F;
            //break;
        }
            
        case tok_ident:
            return ParseIdentity();
            //break;
        case '{':
            getnexttoken();
            return nullptr;
        case '=':
            getnexttoken();
            return ParseExpression();
            //break;
        default:
            getnexttoken();
            return nullptr;
    }
}

 std::vector<std::unique_ptr<Expr>> ParseScope()
{
    getnexttoken();

    
    std::vector<std::unique_ptr<Expr>> vec;
    while(curtok != '}')
    {
        auto A = ParseAny();
        if (!!A)
        {
            //LogStatus(stringf("Scope EXPR: %s", (*A).tostring().c_str()).c_str());
            vec.push_back(std::move(A));
        }
        if (!A)
        {
            //LogStatus("Scope Expr is null");
        }
        
        if (curtok == -1)
        {
            LogError("Scope range extended to EOF");
            return {};
        }
        //LogStatus(stringf("Token: %i", curtok).c_str());
    }
    //auto V = std::make_unique<std::vector<Expr>>(vec);
    //LogStatus(stringf("Scope Expr body size: %zu", V->size()).c_str());
    return vec;
}

#pragma endregion


#pragma region "IR generator"
std::string IR = "";




#pragma endregion // End IR generator region


int compile()
{
    while(true)
    {
        switch(curtok)
        {
            case EOF:
                return EOF;
            case tok_new:
                ParseDefinition();
                break;
            case tok_func:
            {
                auto F = ParseFunction();
                LogStatus(stringf("Function: %s", F->tostring().c_str()).c_str());
                break;
            }
                
            case tok_ident:
                ParseIdentity();
                break;
            case '{':
                getnexttoken();
            case '=':
                getnexttoken();
                ParseExpression();
                break;
            default:
                getnexttoken();
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    newlevel("new h = 3+6");

    binopmap["+"] = 10;
    binopmap["-"] = 10;
    binopmap["*"] = 30;
    binopmap["/"] = 30;
    binopmap["^"] = 40;
    
    auto start = c::high_resolution_clock::now();
    compile();
    auto end = c::high_resolution_clock::now();
    fprintf(stderr, "Compile time took %0.2fμs\n", c::duration<float, c::microseconds::period>(end-start).count());
}