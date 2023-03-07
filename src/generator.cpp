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
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <iomanip>

#define _CRT_SECURE_NO_WARNINGS

namespace c = std::chrono;
using std::unique_ptr; using std::make_unique;

#if defined(NDEBUG)
const bool __verbose = false;
#else
const bool __verbose = true;
#endif

bool verbose = true && __verbose;

unsigned int g_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
inline long double fastrand() { 
  g_seed = (214013*g_seed+2531011); 
  return (long double)((g_seed>>16)&0x7FFF) / (long double)0x7FFF; 
} 

FILE* __logfile = NULL;
/*template<typename ... Args>
void* Logf(std::string fmt, Args ... args)
{
    if (__logfile == nullptr)
    {
        __logfile = fopen("log.txt", "w");
    }
    auto t = std::time(nullptr);
    tm tm;
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    fprintf(__logfile, "[%s]: %s\n", oss.str().c_str(), stringf(fmt.c_str(), args...).c_str());
    #if !defined(NPRINT)
    fprintf(stderr, "[%s]: %s\n", oss.str().c_str(), stringf(fmt.c_str(), args...).c_str());
    #endif

    return nullptr;
}*/
void* Logf(std::string fmt)
{
    if (__logfile == nullptr)
    {
        fopen_s(&__logfile, "log.txt", "w");
    }
    auto t = std::time(nullptr);
    tm tm;
    localtime_s(&tm, &t);
    std::ostringstream oss;
    
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    fprintf(__logfile, "[%s]: %s\n", oss.str().c_str(), fmt.c_str());
    #if !defined(NPRINT)
    fprintf(stderr, "[%s]: %s\n", oss.str().c_str(), fmt.c_str());
    #endif

    return nullptr;
}

#pragma region "Lexer"

std::vector<int> blacklist = {'.', '_', '(', ')', '{', '}', '[', ']', EOF};

std::string file;
size_t fiindex = 0;
int lastchar = ' ';
int curtok = ' ';
std::string identstr = "";
std::string numstr = "";
std::string opstr = "";
std::string stringstr = "";
long long column = -1;
size_t line = 1;

bool blacklisted(int c)
{
    for (const auto bc : blacklist)
    {
        if (bc == c)
            return true;
    }
    return false;
}

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
        column = 0;
        ++line;
    }
    return c;
}
static int reverselexer()
{
    --fiindex;

    --column;
    if (column < 1)
    {
        --line;
        column = 1;
    }

    return file[fiindex];
}

static int gettok()
{
    opstr = "";
    while(isspace(lastchar))
        lastchar = getnext();
    
    if (isalpha(lastchar))
    {
        identstr = "";
        
        do
        {
            identstr += lastchar;
        } while (isalnum((lastchar = getnext())) || lastchar == '_');

        for (const auto& [name, tok] : vtex::keys)
        {
            if (strcmp(name, identstr.c_str()) == 0)
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

    if (!isalnum(lastchar) && !blacklisted(lastchar))
    {
        opstr = "";
        while(true)
        {
            opstr += lastchar;
            lastchar = getnext();
            if (isalnum(lastchar) || isspace(lastchar) || blacklisted(lastchar))
            {
                if (opstr.size() == 1)
                    curtok = opstr.at(0);
                if (opstr == "//")
                    return tok_1lc;
                if (opstr == "/*")
                    return tok_mlc;
                if (opstr == "*/")
                    return tok_mlce;
                
                return tok_op;
            }
        }
    }

    if (lastchar == EOF)
        return EOF;

    if (lastchar == '\n' || lastchar == '\r')
        printf("Bruh\n");
    auto thischar = lastchar;
    lastchar = getnext();
    return thischar;
}

static int getnexttoken()
{
    return (curtok = gettok());
}
static int reversetoken()
{
    reverselexer();
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

typedef unique_ptr<Expr> uExpr;

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

class BooleanExpr : public Expr
{
    vtex::Boolean Val;
    public:
        BooleanExpr(vtex::Boolean Val) : Val(Val) {}
        std::string tostring() override
        {
            return Val.tostring();
        }
        int codegen() override;
};

class VariableExpr : public Expr
{
    std::string Name = "";
    //bool isnan = false;
    public:
        VariableExpr(std::string Name) : Name(Name) {}
        std::string tostring() override
        {
            return "@"+Name;
        }
        int codegen() override;
};

class VsetExpr : public Expr
{
    uExpr Var = nullptr;
    uExpr E = nullptr;
    public:
        VsetExpr(uExpr Var, uExpr E) : Var(std::move(Var)), E(std::move(E)) {}
        std::string tostring() override
        {
            if (!Var || !E)
                return "__null";
            
            std::string str = Var->tostring()+" = "+E->tostring();
            return str;
        }
};

class ValueExpr : public Expr
{
    std::unique_ptr<vtex::Value> Val;
    public:
        ValueExpr(std::unique_ptr<vtex::Value> Val) : Val(std::move(Val)) {}
        std::string tostring() override
        {
            if (!Val)
                return "__null";
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
        if (!LHS || !RHS)
            return "__null";
        return stringf("(%s %s %s)", LHS->tostring().c_str(), Op.c_str(), RHS->tostring().c_str());
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

class NullExpr : public Expr
{
    public:
        NullExpr() {}
        std::string tostring() override {return "__null";}
        int codegen() override {return 0;}
};

class BodyExpr : public Expr
{
    std::vector<std::unique_ptr<Expr>> Body;
    public:
        BodyExpr(std::vector<std::unique_ptr<Expr>>&& body)
        {
            for (auto& h : body)
            {
                Body.push_back(std::move(h));
            }
        }
        std::string tostring() override
        {
            std::string str = "";
            int a = 0;
            for (auto& h : Body)
            {
                if (a > 0)
                str+="\n";
                if (!!h)
                    str+="\t"+h->tostring();
                ++a;
            }
            return str;
        }
};

class ReturnExpr : public Expr
{
    unique_ptr<Expr> Ret = nullptr;
    public:
        ReturnExpr(unique_ptr<Expr> Ret) : Ret(std::move(Ret)) {}
        std::string tostring() override
        {
            if (!Ret)
                return "__null";
            std::string str = "return "+Ret->tostring();
            return str;
        }
};

class IfExpr : public Expr
{
    unique_ptr<Expr> Condition = nullptr;
    unique_ptr<Expr> Next = nullptr;
    unique_ptr<Expr> Else = nullptr;
    public:
        IfExpr(unique_ptr<Expr> Condition, unique_ptr<Expr> Next, unique_ptr<Expr> Else) : Condition(std::move(Condition)), Next(std::move(Next)), Else(std::move(Else)) {}
        std::string tostring() override
        {
            if (!Condition || !Next)
                return "__null";
            std::string str = "if("+Condition->tostring()+"):\n"+Next->tostring();
            if (!!Else)
                str+="\n"+Else->tostring();
            return str;
        }
};

class ElseExpr : public Expr
{
    unique_ptr<Expr> Next = nullptr;
    public:
        ElseExpr(unique_ptr<Expr> Next) : Next(std::move(Next)) {};
        std::string tostring() override
        {
            if (!Next)
                return "__null";
            std::string str = "else:\n"+Next->tostring();
            return str;
        }
};

class WhileExpr : public Expr
{
    uExpr Condition = nullptr;
    uExpr Next = nullptr;
    //unique_ptr<Expr> Atend = nullptr;
    public:
        WhileExpr(uExpr Condition, uExpr Next) : Condition(std::move(Condition)), Next(std::move(Next)) {}
        std::string tostring() override
        {
            if (!Condition || !Next)
            {
                return "__null";
            }
            std::string str = "while ("+Condition->tostring()+"):\n"+Next->tostring();
            return str;
        }
};

class BreakExpr : public Expr
{
    public:
        BreakExpr() {}
        std::string tostring() override
        {
            return "break";
        }
};

class ForExpr : public Expr
{
    uExpr Var = nullptr;
    uExpr Start = nullptr;
    uExpr Iters = nullptr;
    uExpr Iter = nullptr;
    uExpr Next = nullptr;
    public:
        ForExpr(uExpr Var, uExpr Start, uExpr Iters, uExpr Iter, uExpr Next) : Var(std::move(Var)), Start(std::move(Start)),
            Iters(std::move(Iters)), Iter(std::move(Iter)), Next(std::move(Next)) {}
        std::string tostring() override
        {
            if (!Var || !Start || !Iters || !Iter)
            {
                return "__null";
            }
            std::string str = "for ("+Var->tostring()+", "+Start->tostring()+", "+Iters->tostring()+", "+Iter->tostring()+"):\n"+Next->tostring();
            return str;
        }
};

class FunctionExpr : public Expr
{
    std::unique_ptr<Expr> Proto = nullptr;
    unique_ptr<Expr> Body = nullptr;
    public:
        FunctionExpr(std::unique_ptr<Expr> proto, unique_ptr<Expr> body) 
        {
            if (!!proto)
                Proto = std::move(proto);
            if (!!body)
                Body = std::move(body);
        }
    std::string tostring() override
    {
        std::string str = "";
        if (!!Proto)
            str+=Proto->tostring();

        str+="\n{\n";

        if (!!Body)
            str+=Body->tostring();
        str+="\n}";
        return str;
    }
};

class CalleeExpr : public Expr
{
    std::string Fname = "";
    std::vector<std::unique_ptr<Expr>> Args;
    public:
        CalleeExpr(std::string fname, std::vector<std::unique_ptr<Expr>>&& args) : Fname(fname)
        {
            for (auto& uptr : args)
            {
                Args.push_back(std::move(uptr));
            }
        }
        std::string tostring()
        {
            std::string out = Fname;
            int a = 0;
            for (const auto& uptr : Args)
            {
                if (a == 0)
                out += " < ";
                else
                out += ", ";
                if (!!uptr)
                    out+=uptr->tostring();
                ++a;
            }
            return out;
        }
};

#pragma endregion // End AST region


#pragma region "Parser"

std::unordered_map<std::string, int> usrvarmap;
std::vector<std::unordered_map<std::string, int>> ScopeMap;
int scope = 0;
bool ErrorOccurred = false;

int RaiseScope()
{   
    if (scope < 1000)
    ++scope;
    return scope;
}

int LowerScope()
{
    if (scope > 0)
    --scope;
    return scope;
}

/*
std::unique_ptr<Expr> getvar(std::string name)
{
    if (ScopeMap.size() == 0)
    {
        return std::move(usrvarmap.at(name));
    } else
    {

    }
}*/

std::unordered_map<std::string, int> binopmap;

std::unique_ptr<Expr> LogError(const char* str)
{
    //if (verbose)
    fprintf(stderr, "ERROR [Ln %zu, Col %lld]: %s\n", line, column, str);
    ErrorOccurred = true;
    return nullptr;
}
std::unique_ptr<Expr> LogStatus(const char* str)
{
    //#if !defined(NDEBUG)
    if (verbose)
    fprintf(stderr, "[Ln %zu, Col %lld]: %s\n", line, column, str);
    //#endif
    return nullptr;
}
std::unique_ptr<Expr> LogNote(const char* str)
{
    //if (verbose)
    fprintf(stderr, "Note [Ln %zu, Col %lld]: %s\n", line, column, str);
    return nullptr;
}

bool varexists(std::string str)
{   
    if (ScopeMap.size() > 0)
    for (auto i = ScopeMap.rbegin(); i != ScopeMap.rend(); ++i)
    {
        auto itr = i->find(str);
        if (itr != i->end())
            return true;
    }

    return (usrvarmap.find(str) != usrvarmap.end());
}

void putvar(std::string name)
{
    if (ScopeMap.size() == 0)
    {
        //LogStatus("Put in globmap");
        usrvarmap[name] = 0;
    } else
    {
        //LogStatus("Put in a scopemap");
        ScopeMap.back()[name] = 0;
    }
}

void putglobvar(std::string name)
{
    usrvarmap[name] = 0;
}

std::unique_ptr<Expr> ParseScope();
std::unique_ptr<Expr> ParseIdentity();
std::unique_ptr<Expr> ParseIf();
uExpr ParseWhile();

std::unique_ptr<vtex::Value> ParseString()
{
    stringstr = "";
    while((curtok = getnext()) != '\"' && curtok != EOF)
    {
        stringstr += curtok;
    }
    if (curtok == EOF)
    {
        LogError("String literall extends to EOF");
        return nullptr;
    }

    lastchar = getnexttoken(); // Eat ending '"' and restore the lexer
    return std::make_unique<vtex::Value>(std::make_unique<vtex::String>(stringstr));
}

std::unique_ptr<Expr> ParseDefinition()
{
    getnexttoken(); // Eat "new"

    if (curtok != tok_ident && curtok != tok_glob)
        return LogError("Expected an identifier after \"new\"");
    
    bool useglob = false;
    if (curtok == tok_glob)
    {
        useglob = true;
        LogStatus("Putting variable to global");
        getnexttoken();
    }
    std::string name = identstr;
    if (varexists(name))
    {
        LogNote(stringf("Redifinition of variable \"%s\"", name.c_str()).c_str());
    }
    if (!useglob)
    putvar(name);
    else
    putglobvar(name);

    //usrvarmap[name.c_str()] = 0;
    LogStatus(stringf("New variable \"%s\"", name.c_str()).c_str());

    //getnexttoken(); // Eat name
    
    return std::make_unique<VariableExpr>(name.c_str());
}

std::unique_ptr<ProtoExpr> ParsePrototype()
{
    getnexttoken(); // Eat "function"
    //LogStatus("Found function specifier");
    std::string name = "";
    if (curtok == tok_ident)
    {
        LogStatus("Found function variable");
        name = identstr;
        getnexttoken();
    } else
    {
        LogStatus("Found first class function");
        name = "__anon_function";
    }

    if (curtok != '(')
    {
        LogError("Expected '(' after function specifier");
        return nullptr;
    }
    getnexttoken();
    std::vector<std::string> argnames = {};
    std::unordered_map<std::string, int> map;
    
    while(curtok != ')')
    {
        if (curtok != tok_ident)
        {
            LogError("Expected identifier in prototype argument definitions");
            return nullptr;
        }
        LogStatus(stringf("Function parameter: \"%s\"", identstr.c_str()).c_str());
        putvar(identstr);
        //argnames.push_back(identstr);
        //map[identstr] = 0;//std::make_unique<VariableExpr>(identstr);
        getnexttoken();
        
        if (curtok != ',' && curtok == ')')
        {
            break;
        } else if (curtok != ',' && curtok != ')')
        {
            LogError("Expected ',' or ')' in prototype argument definitions");
            return nullptr;
        }
        getnexttoken();
    }
    putvar("__function:"+name);
    //ScopeMap.push_back(map);
    return std::make_unique<ProtoExpr>(name, argnames);
}

std::unique_ptr<Expr> ParseFunction()
{
    std::unique_ptr<ProtoExpr> P = ParsePrototype();
    std::unique_ptr<Expr> B;

    getnexttoken();
    if (curtok != '{' && !!P)
    {
        LogNote("Function has no scope");
        //ScopeMap.pop_back();
        return std::make_unique<FunctionExpr>(std::move(P), (std::unique_ptr<Expr>)nullptr);
    } else if (!!P)
    {
        B = ParseScope();
        //ScopeMap.pop_back();
        //if (B.size() != 0)
        {
            return std::make_unique<FunctionExpr>(std::move(P), std::move(B));
        }
        LogError("Scope body is nullptr, proceeding with proto only");
        return std::make_unique<FunctionExpr>(std::move(P), (std::unique_ptr<Expr>)nullptr);
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
            {
                auto S = std::make_unique<ValueExpr>(ParseString());
                getnexttoken();
                return std::move(S);
            }
        
        case tok_if:
            return ParseIf();
        case tok_true:
        case tok_false:
        case tok_ident:
            return ParseIdentity();
        case tok_func:
            return ParseFunction();
        case EOF:
            return nullptr;
        default:
            return LogError(stringf("Unknown token %i in expression", curtok).c_str());
    }
}

int GetBinopPrec(std::string str)
{
    for (const auto& itr : vtex::stdops)
    {
        if (std::get<0>(itr) == str)
        {
            opstr = "";
            return std::get<1>(itr);
        }
    }
    //opstr = "";
    return -1;
}

std::unique_ptr<Expr> ParseRHS(int Expected, std::unique_ptr<Expr> LHS)
{
    while(true)
    {
        if (curtok>=tok_number && curtok<=tok_ident) // If unexpected token, just get rid of it
        {
            getnexttoken(); // hi
        }
        //LogStatus(opstr.c_str());
        auto op = opstr;
        int Prec = GetBinopPrec(op);
        if (Prec < Expected || blacklisted(curtok))
        {
            return LHS;
        }
        
        //op = "";
        getnexttoken(); // Eat binop
        auto RHS = ParsePrimary();
        if (!RHS)
        {
            return LogNote("Right hand symbol (RHS) is null");
        }

        getnexttoken(); // Eat the primary... NOT GOOD, DO NOT USE PLEASE
        //LogStatus(stringf("supposedly an op: %i", curtok).c_str());
        //op = opstr;

        // If the next binop has more precedence, give it RHS with a higher expected precedence
        int Next = GetBinopPrec(op);

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
        //LogStatus(stringf("RHS end at token: %i", curtok).c_str());
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
    //getnexttoken();

    RHS = ParseRHS(0, std::move(LHS));
    if (!!RHS)
    {
        LogStatus(stringf("RHS = %s", RHS->tostring().c_str()).c_str());
    }
    return RHS;
}

std::unique_ptr<Expr> ParseFcall(std::string fname)
{
    getnexttoken(); // Eat '('

    std::vector<std::unique_ptr<Expr>> argvec;

    while (curtok != ')')
    {
        if (curtok == EOF)
        {
            return LogError("Function argument list extends to EOF");
        }
        //LogStatus(stringf("Token in fcall: %s", numstr.c_str()).c_str());
        auto arg = ParseExpression();
        if (!arg)
        {
            return LogError("Function argument is null");
        }
        //LogStatus(arg->tostring().c_str());
        //getnexttoken(); // Eat argument expression

        if (curtok != ',' && curtok != ')')
        {
            return LogError(stringf("Expected continuation or end of argument list, but got token %i", curtok).c_str());
        }
        argvec.push_back(std::move(arg));
        if (curtok == ',')
        {
            getnexttoken();
        }
    }
    auto FC = std::make_unique<CalleeExpr>(fname, std::move(argvec));
    if (!!FC)
    {
        LogStatus(stringf("Function call: %s", FC->tostring().c_str()).c_str());
    }
    getnexttoken();
    return std::move(FC);
}

std::unique_ptr<Expr> ParseSet(std::string name)
{
    //getnexttoken(); // Eat "="

    auto LHS = ParseExpression();
    if (LHS != nullptr)
    {
        //LogStatus(stringf("LHS = %s", LHS->tostring().c_str()).c_str());
    }
    
    return nullptr;
}

std::unique_ptr<Expr> ParseIdentity()
{
    auto lident = identstr;

    switch(curtok)
    {
        case tok_true:
            {
                //auto B = std::make_unique<BooleanExpr>(vtex::Boolean(true));
                auto B = std::make_unique<ValueExpr>(std::make_unique<vtex::Value>(std::make_unique<vtex::Boolean>(true)));
                getnexttoken();
                return std::move(B);
            }
            
        case tok_false:
            {
                auto B = std::make_unique<ValueExpr>(std::make_unique<vtex::Value>(std::make_unique<vtex::Boolean>(false)));
                getnexttoken();
                return std::move(B);
            }
        case tok_ident:
            getnexttoken();
            break;
    }
    
    //LogStatus(stringf("Token after ident: %i", curtok).c_str());
    switch(curtok)
    {
        case '(':
            if (!varexists(("__function:"+lident)))
            {
                return LogError(stringf("Function name \"%s\" does not exist", ("__function:"+lident).c_str()).c_str());
            }
            LogStatus(stringf("Function call on variable \"%s\"", ("__function:"+lident).c_str()).c_str());
            return ParseFcall("__function:"+lident);

        default:
        {
            if (!varexists(lident))
                return LogError(stringf("Unknown identity \"%s\" in expression", lident.c_str()).c_str());
            /*for (const auto op : vtex::SetOps)
            {
                if (op == opstr)
                {
                    if (!varexists(lident))
                    {
                        return LogError(stringf("Unknown identity \"%s\" in expression", lident.c_str()).c_str());
                    }
                    LogStatus(stringf("Variable write operation on \"%s\"", lident.c_str()).c_str());
                    getnexttoken();
                    return ParseSet(lident);
                }
            }

            

            LogStatus(stringf("Variable read operation on \"%s\"", lident.c_str()).c_str());*/
            return std::make_unique<VariableExpr>(lident);
        }
    }
}

unique_ptr<Expr> ParseReturn()
{
    getnexttoken(); // Eat "return"

    auto E = ParseExpression();
    if (!E)
    {
        return LogError("Return expression is null");
    }

    auto R = make_unique<ReturnExpr>(std::move(E));
    LogStatus(R->tostring().c_str());
    return std::move(R);
};

bool BREAK = false;
std::unique_ptr<Expr> ParseAny()
{
    switch(curtok)
    {
        case EOF:
            BREAK = true;
            return nullptr;
        case tok_1lc:
            {
                while (true)
                {
                    curtok = getnext();
                    if (curtok == '\n' || curtok == '\r' || curtok == EOF) // Stop at new line, and dont eat the EOF
                        break;
                    //LogStatus(stringf("esh %i", curtok).c_str());
                }
                getnexttoken(); // Eat new line, and restore lexer
            }
            return ParseAny(); // Return expression after comment to fix things
        case tok_mlc:
            {
                while(true)
                {
                    curtok = gettok();
                    if (curtok == tok_mlce || curtok == EOF) // Stop at the multiline comment end token, and dont eat the EOF
                        break;
                }
                getnexttoken(); // Eat multiline comment end, and restore lexer
            }
            return ParseAny(); // Return expression after comment to fix things
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
        case tok_if:
            return ParseIf();
        case tok_while:
            return ParseWhile();
        case tok_break:
            getnexttoken();
            return make_unique<BreakExpr>();
        case tok_ident:
            return ParseExpression();
            //break;
        case tok_ret:
            return ParseReturn();
        case '{':
            return ParseScope();
        case '}':
            return nullptr;
        case tok_op:
            {
                if (opstr == "=")
                {
                    getnexttoken(); // Eat '='
                    auto E = ParseExpression();
                    //getnexttoken();
                    return std::move(E);
                }
            }
            //break;
        default:
            getnexttoken();
            return nullptr;
    }
}

uExpr ParseWhile()
{
    getnexttoken(); // Eat "while"

    if (curtok != '(')
        return LogError("Expected '(' after \"while\"");

    getnexttoken(); // Eat "("

    auto E = ParseExpression();
    if (!E)
    {
        return LogError("While loop argument expression is null");
    }

    if (curtok != ')')
    {
        return LogError("Exprected ')' after while loop argument expression");
    }
    getnexttoken(); // Eat ")"

    auto A = ParseAny();

    if (!A)
    {
        return LogError("While loop body is null");
    }

    auto WHILE = make_unique<WhileExpr>(std::move(E), std::move(A));
    LogStatus(WHILE->tostring().c_str());
    return std::move(WHILE);
}

std::unique_ptr<Expr> ParseIf()
{
    getnexttoken(); // Eat "if"

    if (curtok != '(')
    {
        return LogError("Expected '(' after \"if\"");
    }

    getnexttoken(); // Eat "("

    auto E = ParseExpression();

    if (!E)
    {
        LogError("If statement argument expression is null");
    }

    if (curtok != ')')
    {
        return LogError("Expected ')' to end if statement argument expression");
    }
    getnexttoken(); // Eat ")"

    auto A = ParseAny();

    if (!A)
    {
        LogError("If statement body is null");
    }

    unique_ptr<Expr> ELSE = nullptr;

    if (curtok == tok_else)
    {
        getnexttoken(); // Eat "else"
        ELSE = make_unique<ElseExpr>(ParseAny());
    }

    //LogStatus(stringf("Token after if: %i", curtok).c_str());

    auto IF = make_unique<IfExpr>(std::move(E), std::move(A), std::move(ELSE));
    LogStatus(IF->tostring().c_str());
    return std::move(IF);
}

std::unique_ptr<Expr> ParseScope()
{
    getnexttoken(); // Eat "{"
    
    std::vector<std::unique_ptr<Expr>> vec;
    std::unordered_map<std::string, int> map;
    ScopeMap.push_back(map);
    while(curtok != '}')
    {
        auto A = ParseAny();
        if (!!A)
        {
            //LogStatus(stringf("Scope EXPR: %s", (*A).tostring().c_str()).c_str());
            vec.push_back(std::move(A));
        }
        if (curtok == '}')
        {
            //Logf("flflfl");
        }
        if (!A)
        {
            //LogStatus("Scope Expr is null");
        }
        
        if (curtok == -1)
        {
            LogError("Scope range extended to EOF");
            ScopeMap.pop_back();
            return {};
        }
        //LogStatus(stringf("Token: %i", curtok).c_str());
        //getnexttoken();
    }
    if (!vec.size())
    {
        auto E = std::make_unique<NullExpr>();
        vec.push_back(std::move(E));
    }
    auto B = std::make_unique<BodyExpr>(std::move(vec));
    LogStatus("End of scope body");
    ScopeMap.pop_back();
    getnexttoken();
    return B;
}

#pragma endregion


#pragma region "IR generator"
std::string IR = "";

int StringExpr::codegen()
{
    return 0;
}

int NumberExpr::codegen()
{
    return 0;
}

int VariableExpr::codegen()
{
    return 0;
}

int ValueExpr::codegen()
{
    return 0;
}


#pragma endregion // End IR generator region


int compile()
{
    while(!BREAK)
    {
        ParseAny();
    }
    return 0;
}

int main(int argc, char* argv[])
{
    std::ifstream f("scripty.vtex");
    std::stringstream stream;
    stream << f.rdbuf();

    newlevel(stream.str());
    f.close();

    //vtex::stdops.push_back({"+", vtex::Value::add});
    binopmap["+"] = 10;
    binopmap["-"] = 10;
    binopmap["*"] = 30;
    binopmap["/"] = 30;
    binopmap["=="] = 5;
    
    auto start = c::high_resolution_clock::now();
    compile();
    auto end = c::high_resolution_clock::now();
    if (ErrorOccurred)
    {
        Logf("Fatal error(s) have occurred while compilation and will not continue.");
        //fprintf(stderr, "Fatal errors have occurred while compilation, will not continue.\n");
    } else
    {
        Logf("Code was compiled successfully and will continue.");
    }
    
    fprintf(stderr, "Compile time took %0.2fus\n", c::duration<float, c::microseconds::period>(end-start).count());

    if (ErrorOccurred)
    return -1;
    
    return 0;
}
