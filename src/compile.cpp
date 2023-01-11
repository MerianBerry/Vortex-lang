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
    ivar = -1,
    itype = -2,
    iop = -3,
    expi = -4,
    ok = 0,

    decl,
    fwhile,
    fif,
    fthen,
    fend,

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

std::vector<std::pair<std::string, exprAST>> keywords =
{
    {"new", declAST()}
};

std::unordered_map<std::string, exprAST> keywordmap(keywords.begin(), keywords.end());

std::vector<std::pair<std::string, exprAST>> stdtypes =
{
    {"Int", intAST(0)},
    {"Float", floatAST(0)},
    {"String", stringAST("")}
};

std::unordered_map<std::string, exprAST> usrtypes(stdtypes.begin(), stdtypes.end());

std::vector<std::unique_ptr<exprAST>> exprstack = {};

static FILE* fi = nullptr;

static std::string identstr = "";
static std::string identnum = "";

static int GetToken()
{
    static int lastchar = ok;

    while(isspace(lastchar))
    {
        lastchar = getc(fi);
    }

    if (isalpha(lastchar))
    {
        identstr = "";
        do
        {
            identstr += (char)lastchar;
        } while (isalnum((lastchar = getc(fi))));
        
        {
            if (identstr == "new")
            {
                return decl;
            }
        }
        {
            if (identstr == "Float")
            {
                return fnum;
            }
        }


        return expi;
    }
}

static std::unique_ptr<exprAST> ParseDecl()
{

}

static std::unique_ptr<exprAST> ParseDouble()
{

}

static void ParseToplevel()
{
    int state = GetToken();
    switch(state)
    {
        case decl:
        {
            exprstack.push_back(ParseDecl());
        }
        break;


        case fnum:
        {
            exprstack.push_back(ParseDouble());
        }
        break;
    }
}

int main(int argc, char* argv[])
{

    return 0;
}