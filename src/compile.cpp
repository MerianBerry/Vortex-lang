// C headers
#include <cstdio>
#include <cctype>
#include <cstring>
// CXX headers
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <any>

enum TOKEN
{
    ivar = -2,
    itype = -3,
    iop = -4,
    expi = -5,
    ok = 0,
    strtexpr,
    endexpr,

    vset,
    val,
    ident,

    decl,
    fwhile,
    fif,
    fthen,
    fend,

    type,
    fnum,
    str
};

//-- ABSTRACT PARSING TREE, or my weird version of one --//
class exprAST
{
    public:
    virtual ~exprAST() = default;
};

class declAST : public exprAST
{
    std::unique_ptr<exprAST> typeval;
    std::unique_ptr<exprAST> var;
    public:
    declAST() {}
    declAST(std::unique_ptr<exprAST> tval, std::unique_ptr<exprAST> var) : typeval(std::move(tval)), var(std::move(var)) {}
};
class enddeclAST : public exprAST
{

};

class intAST : public exprAST
{
    long long val;
    public:
    intAST(const long long& val) : val(val) {}
};

class floatAST : public exprAST
{
    double val;
    public:
    floatAST(const double& _val) : val(_val) {}
};

class stringAST : public exprAST
{
    std::string val;
    public:
    stringAST(const std::string& val) : val(val) {}
};

class vardeclAST : public exprAST
{
    std::string name;
    bool initialized;
    public:
    vardeclAST(const std::string& name, const bool& initialized) : name(name), initialized(initialized) {}
};

std::vector<std::pair<std::string, int>> keywords =
{
    {"new", decl}
};

std::unordered_map<std::string, int> keywordmap(keywords.begin(), keywords.end());

std::vector<std::pair<std::string, exprAST>> stdtypes =
{
    {"Int", intAST(0)},
    {"Float", floatAST(0)},
    {"String", stringAST("")}
};

std::unordered_map<std::string, exprAST> usrtypes(stdtypes.begin(), stdtypes.end());

std::vector<int> exprstack = {};

static FILE* fi = nullptr;

static std::string identstr = "";
static std::string identnum = "";

static int GetToken()
{
    static int lastchar = ok;
    static int laststate = ok;

    while(isspace(lastchar))
    {
        lastchar = getc(fi);
    }

    if (lastchar == '=')
    {
        lastchar = getc(fi);
        return vset;
    }

    if (isalpha(lastchar))
    {
        identstr = "";
        do
        {
            identstr += (char)lastchar;
        } while (isalnum((lastchar = getc(fi))));
        
        lastchar = getc(fi);
        {
            if (identstr == "new")
            {
                
                return decl;
            }
        }
        {
            if (identstr == "Float")
            {
                return type;
            }
        }

        return ident;
    }

    if (isdigit(lastchar))
    {
        lastchar = getc(fi);
        return val;
    }
    if (lastchar == EOF)
    {
        return EOF;
    }
    lastchar = getc(fi);
    return strtexpr;
}

static std::unique_ptr<exprAST> ParseDecl()
{
    return nullptr;
}

static std::unique_ptr<exprAST> ParseDouble()
{
    return nullptr;
}

static int ParseToplevel()
{
    int state = GetToken();
    exprstack.push_back(state);
    
    switch(state)
    {
        case decl:
        {
            //exprstack.push_back(ParseDecl());
            
        }
        break;


        case fnum:
        {
            //exprstack.push_back(ParseDouble());
        }
        break;
    }
    return state;
}

int main(int argc, char* argv[])
{
    fi = fopen("test.vtex", "r");
    if (fi == NULL)
    {
        return -1;
    }
    while(true)
    {
        if (ParseToplevel() < 0)
        {
            break;
        }
    }
    for (const auto& token : exprstack)
    {
        printf("Expr tokens: %i\n", token);
    }
    fclose(fi);
    return 0;
}