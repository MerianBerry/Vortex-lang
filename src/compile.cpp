// C headers
#include <cstdio>
#include <cctype>
#include <cstring>
// CXX headers
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

enum TOKEN
{
    ivar = -2,
    itype = -3,
    iop = -4,
    expi = -5,
    ok = 0,
    strtexpr,
    endexpr,

    vwrite,
    vread,
    val,
    ident,
    cont,
    op,

    decl,
    var,
    fwhile,
    fif,
    fthen,
    fend,

    type,
    num,
    str
};

std::vector<uint32_t> valstack = {};

class exprNODE
{
    public:
    virtual ~exprNODE() = default;
    virtual int gettype() {return ok;}
    virtual int getnext() {return ok;}
    virtual std::string getname() {return "Blank expression";}
    virtual void setnext(std::unique_ptr<exprNODE> next) {}
    virtual void reconstruct(const std::string& _val) {}
    virtual std::string getval() {return "";}
};

class vwriteNODE : public exprNODE
{
    int toktype = vwrite;
    std::unique_ptr<exprNODE> next;
    public:
    vwriteNODE() {}
    int gettype() override {return toktype;}
    int getnext() override {return val;}
    std::string getname() override {return "Variable write expression";}
    void setnext(std::unique_ptr<exprNODE> next) override {next = std::move(next);}
};

class tNODE : public exprNODE
{
    std::string typestr = "";
    int type = ok;
    int toktype = TOKEN::type;
    public:
    tNODE(const std::string& typestr) : typestr(typestr) {}
    int gettype() override {return toktype;}
    int getnext() override {return var;}
    std::string getname() override {return "Type expression";}
};

class varNODE : public exprNODE
{
    std::string name = "";
    std::unique_ptr<exprNODE> next;
    int toktype = var;
    public:
    varNODE(const std::string& name) : name(name) {}
    varNODE(std::unique_ptr<exprNODE> next) : next(std::move(next)) {}
    varNODE() {}
    int gettype() override {return toktype;}
    int getnext() override {return vwrite;}
    std::string getname() override {return "Variable expression";}
    void setnext(std::unique_ptr<exprNODE> next) override {next = std::move(next);}
};
/*
class typeNODE : public tNODE
{
    std::string typestr = "";
    std::unique_ptr<varNODE> next;
    int toktype = TOKEN::type;
    public:
    typeNODE(const std::string& typestr) : typestr(typestr) {}
    typeNODE(std::unique_ptr<varNODE> next) : next(std::move(next)) {}
    typeNODE() {}
    int gettype() override {return toktype;}
    int getnext() override {return var;}
    std::string getname() override {return "Type expression";}
    void setnext(std::unique_ptr<exprNODE> next) override {next = std::move(next);}
};*/

class declNODE : public exprNODE
{
    std::unique_ptr<tNODE> next;
    int toktype = decl;
    public:
    declNODE(std::unique_ptr<tNODE> next) : next(std::move(next)) {}
    declNODE() {}
    int gettype() override {return toktype;}
    int getnext() override {return type;}
    std::string getname() override {return "Decleration expression";}
    void setnext(std::unique_ptr<exprNODE> next) override {next = std::move(next);}
};

class valNODE : public exprNODE
{
    std::string val;
    int toktype = TOKEN::val;
    public:
    valNODE(const std::string& val) : val(val) {}
    int gettype() override {return toktype;}
    int getnext() override {return ok;}
    std::string getname() override {return "Value expression";}
    std::string getval() override {return val;}
};

class floatNODE : public exprNODE
{
    long double val;
    std::unique_ptr<exprNODE> next;
    int toktype = type;
    uint32_t argsize = 1;
    public:
    floatNODE(std::unique_ptr<exprNODE> next) : next(std::move(next)) {}
    floatNODE(const std::string& val) : val(std::strtold(val.c_str(), nullptr)) {valstack.push_back(argsize);}
    floatNODE(std::unique_ptr<exprNODE> next, const std::string& val) : next(std::move(next)), val(strtold(val.c_str(), nullptr)) {}
    int gettype() override {return toktype;}
    int getnext() override {return var;}
    std::string getname() override {return "Float expression";}
    void setnext(std::unique_ptr<exprNODE> next) override {next = std::move(next);}
    void reconstruct(const std::string& _val) override {val = strtold(_val.c_str(), nullptr);}
};

class intNODE : public exprNODE
{
    long long val;
    std::unique_ptr<exprNODE> next;
    int toktype = type;
    uint32_t argsize = 1;
    public:
    intNODE(std::unique_ptr<exprNODE> next) : next(std::move(next)) {}
    intNODE(const std::string& val) : val(strtoll(val.c_str(), nullptr, 10)) {valstack.push_back(argsize);}
    intNODE(std::unique_ptr<exprNODE> next, const long long& val) : next(std::move(next)), val(val) {}
    int gettype() override {return toktype;}
    int getnext() override {return var;}
    std::string getname() override {return "Int expression";}
    void setnext(std::unique_ptr<exprNODE> next) override {next = std::move(next);}
    void reconstruct(const std::string& _val) override {val = strtoll(_val.c_str(), nullptr, 10);}
};

class stringNODE : public exprNODE
{
    std::string val;
    std::unique_ptr<exprNODE> next;
    int toktype = type;
    uint32_t argsize = 1;
    public:
    stringNODE(std::unique_ptr<exprNODE> next) : next(std::move(next)) {}
    stringNODE(const std::string& val) : val(val) {valstack.push_back(argsize);}
    stringNODE(std::unique_ptr<exprNODE> next, const std::string& val) : next(std::move(next)), val(val) {}
    int gettype() override {return toktype;}
    int getnext() override {return var;}
    std::string getname() override {return "String expression";}
    void setnext(std::unique_ptr<exprNODE> next) override {next = std::move(next);}
    void reconstruct(const std::string& _val) override {val = _val;}
};

class VariableConstruct
{
    std::unique_ptr<exprNODE> typenode;
    std::unique_ptr<exprNODE> varnode;
    public:
    VariableConstruct(std::unique_ptr<exprNODE> && newtype) : typenode(std::move(newtype)) {}
    void settypenode(std::unique_ptr<exprNODE> && newtype) {typenode = std::move(newtype);}
    void setvarnode(std::unique_ptr<exprNODE> && newvar) {varnode = std::move(varnode);}
    void Construct(const std::string val)
    {
        typenode->reconstruct(val);
        varnode->setnext(std::move(typenode));
    }
    ~VariableConstruct() = default;
};

std::vector<std::pair<std::string, int>> keywords =
{
    {"new", decl}
};

std::unordered_map<std::string, int> keywordmap(keywords.begin(), keywords.end());



std::unordered_map<std::string, int> usrtypes;

std::vector<std::pair<std::string, int>> opwords =
{
    {"+", 1},
    {"-", 1},
    {"*", 1},
    {"/", 1}
};


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
        return vwrite;
    }
    if (lastchar == '$')
    {
        lastchar = getc(fi);
        return vread;
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
                printf("decl name\n");
                return decl;
            }
        }
        {
            int ret = ok;
            auto itr = usrtypes.find(identstr);
            if (itr != usrtypes.end())
            {
                ret = type;
            }
            if (identstr == "Float")
            {
                //return type;
            }
            if (ret != 0)
                return ret;
        }

        return ident;
    }

    if (isdigit(lastchar) || lastchar == '.')
    {
        identnum = "";
        do
        {
            identnum += (char)lastchar;
        } while(isdigit((lastchar = getc(fi))) || lastchar == '.');
        lastchar = getc(fi);
        return num;
    }
    if (lastchar == EOF)
    {
        return EOF;
    }
    lastchar = getc(fi);
    return ok;
}

std::vector<int> exprstack = {1};
std::vector<std::unique_ptr<exprNODE>> exprnodes = {};

bool PushToNodeStack(std::unique_ptr<exprNODE> node)
{
    if (exprnodes.size() != 0)
    {
        auto nodet = std::move(exprnodes.at(exprnodes.size()-1));
        //bool caninsert = nodet->getnext() == 0 || nodet->getnext() == node->gettype();
        if (true)
        {
            printf("Can insert next element: %s\n", node->getname().c_str());
            
            exprnodes.push_back(std::move(node));
        }
        return true;
    }
    printf("EXPR node stack is empty, can insert next element: %s\n", node->getname().c_str());
    exprnodes.push_back(std::move(node));
    return true;

}

std::vector<std::unique_ptr<VariableConstruct>> constructor = {};

static int ParseToplevel()
{
    int state = GetToken();
    static int laststate = ok;

    if (state != 0)
        exprstack.push_back(state);
    switch(state)
    {
        case num:
        {
            printf("Identnum: %s\n", identnum.c_str());
            auto node = std::make_unique<valNODE>(identnum);
            if (!PushToNodeStack(std::move(node)))
            {
                valstack.erase(valstack.begin()+0);
            } else
            {
                constructor.at(0)->Construct(identnum);
            }
        }
        break;
        case vwrite:
        {
            std::unique_ptr<exprNODE> node = std::make_unique<vwriteNODE>();
            PushToNodeStack(std::move(node));
        }
        break;
        case type:
        {
            
            printf("Identstr: %s\n", identstr.c_str());
            //auto node = &();
            switch(usrtypes.at(identstr))
            {
                case num:
                {
                    if (PushToNodeStack(std::forward<std::unique_ptr<exprNODE>>(std::move(std::make_unique<floatNODE>("0.0")))))
                    {
                        //auto ptr = std::make_unique<VariableConstruct>();
                        auto construct = std::make_unique<VariableConstruct>(std::move(exprnodes.at(exprnodes.size()-1)));
                        construct->settypenode(std::move(exprnodes.at(exprnodes.size()-1)));
                        constructor.push_back(std::move(construct));
                    }
                }
                break;
                default:
                break;
            }
        }
        break;
        case decl:
        {
            std::unique_ptr<exprNODE> node = std::make_unique<declNODE>();
            PushToNodeStack(std::move(node));
        }
        break;
        case ident:
        {
            printf("Identstr: %s\n", identstr.c_str());
            //std::unique_ptr<exprNODE> node = std::make_unique<varNODE>(identstr);
            if (!PushToNodeStack(std::make_unique<varNODE>(identstr)))
            {
                printf("Unable to push variable identitiy\n");
            } else
            {
                constructor.at(0)->setvarnode(std::move(exprnodes.at(exprnodes.size()-1)));
                //constructor.at(0).settypenode(std::move(exprnodes.at(exprnodes.size()-1)));
            }
        }
        break;
        default:
        break;
    }
    laststate = state;
    return state;
}

int main(int argc, char* argv[])
{
    auto expr = std::make_unique<exprNODE>();
    PushToNodeStack(std::move(expr));
    
    {
        usrtypes["Float"] = num;
        usrtypes["String"] = str;
    }

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
        //printf("Expr tokens: %i\n", token);
    }
    fclose(fi);
    return 0;
}