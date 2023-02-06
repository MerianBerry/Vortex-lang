// Vortex headers
#include "types.h"
#include "operators.h"
#include "tokens.h"
#include "keywords.h"

// C headers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>

// CXX headers
#include <chrono> // For duration diagnostics
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <unordered_map>
#include <stdexcept>

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
        virtual std::string tostring() = 0;
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
};

class VariableExpr : public Expr
{
    std::string Name = "";
    public:
        VariableExpr(std::string Name) : Name(Name) {}
        std::string tostring()
        {
            return Name;
        }
};

#pragma endregion // End AST region


#pragma region "Parser"

static std::unordered_map<std::string, int> usrvarmap;

static std::unique_ptr<Expr> LogError(const char* str)
{
    fprintf(stderr, "ERROR [Ln %zu, Col %lld]: %s\n", line, column, str);
    return nullptr;
}
static std::unique_ptr<Expr> LogStatus(const char* str)
{
    fprintf(stderr, "Status [Ln %zu, Col %lld]: %s\n", line, column, str);
    return nullptr;
}

static std::unique_ptr<Expr> ParseString()
{
    stringstr = "";
    while((curtok = getnext()) != '\"' && curtok != EOF)
    {
        stringstr += curtok;
    }
    if (curtok == EOF)
        return LogError("string literall extends to EOF");

    lastchar = getnexttoken(); // Eat ending '"' and restore the lexer
    return std::make_unique<StringExpr>(vtex::String(stringstr));
}

static std::unique_ptr<Expr> ParseDefinition()
{
    getnexttoken(); // Eat "new"

    if (curtok != tok_ident)
        return LogError("expected an identifier after \"new\"");
    
    usrvarmap[identstr.c_str()] = 0;
    LogStatus(stringf("New variable \"%s\"", identstr.c_str()).c_str());

    return std::make_unique<VariableExpr>(identstr.c_str());
}

static std::unique_ptr<Expr> ParseExpression()
{
    std::unique_ptr<Expr> LHS;

    switch(curtok)
    {
        case tok_number:
            LHS = std::make_unique<NumberExpr>(vtex::LFloat(strtold(numstr.c_str(), nullptr)));
            break;
        case tok_string:
            LHS = std::move(ParseString());
            break;
        default:
            return LogError("unknown token in expression");
    }

    return LHS;
}

static std::unique_ptr<Expr> ParseSet()
{
    getnexttoken(); // Eat "="

    auto LHS = ParseExpression();
    if (LHS != nullptr)
    {
        LogStatus(stringf("LHS = %s", LHS->tostring().c_str()).c_str());
    }

    return nullptr;
}

static std::unique_ptr<Expr> ParseIdentity()
{
    auto lident = identstr;

    getnexttoken(); // Eat identity

    switch(curtok)
    {
        case '=':
            ParseSet();
            break;
        default:
            return LogStatus("nothing to parse for identity");
    }

    return nullptr;
}

#pragma endregion


#pragma region "IR generator"

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
            case tok_ident:
                ParseIdentity();
                break;
            default:
                getnexttoken();
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    newlevel("new a = \"hello\" new h = 69.420");

    auto start = c::high_resolution_clock::now();
    compile();
    auto end = c::high_resolution_clock::now();
    fprintf(stderr, "Compile time took %0.2fμs\n", c::duration<float, c::microseconds::period>(end-start).count());
}