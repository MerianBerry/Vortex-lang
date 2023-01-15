// C HEADINGS
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
// CXX HEADINGS
#include <string>
#include <unordered_map>
#include <deque>
#include <functional>

#include <chrono>
namespace c = std::chrono;

//#define NPRINT

#if defined(NDEBUG)
#define NPRINT
#endif
enum
{
    INVAR = -2,
    INTYPE = -3,
    PWORD = 0,
    PNUM,
    PSTRING,
    POTHER,
    PCOMMENT,
    PRCOMMENT
};

struct valNODE
{
    char* val;
    void setval(const char* _val) {val = (char*)calloc(strlen(_val), 1); strcpy(val, _val);}
};

const std::deque<std::pair<const char*, std::function<const char*(const char*, const char*)>>> numops =
{
    {"+", [](const char* lhs, const char* rhs) -> const char*
    {
        long double ret = strtold(lhs, nullptr) + strtold(rhs, nullptr);
        auto str = std::to_string(ret);
        char* nstr = (char*)calloc(str.size(), 1);
        strcpy(nstr, str.c_str());
        return nstr;
    }}
};
struct numberNODE
{
    long double val;
    void constructor(const char* _val)
    {
        val = strtold(_val, nullptr);
    }
    int getoperator(const char* op)
    {
        for (size_t i = 0; i < numops.size(); ++i)
        {
            if (strcmp(op, numops[i].first))
            {
                return i;
            }
        }
        return -1;
    }
    numberNODE() {}
    ~numberNODE() {}
};

struct typeNODE
{
    enum {NUMBER, STRING} enums;
    uint32_t type;
    union
    {
        numberNODE number;
        char* str;
    };
    void setnumber(const char* val) {number.constructor(val);}
    void setstr(const char* val) {str = const_cast<char*>(val);}
    typeNODE() {}
    ~typeNODE() {}
};

struct varNODE
{
    char* name = NULL;
    typeNODE value;
    varNODE() {}
    ~varNODE() {}
};

struct declNODE
{
    declNODE() {}
    ~declNODE() {}
};

struct vwriteNODE
{
    char* varname = NULL;
    vwriteNODE() {}
    ~vwriteNODE() {}
};

struct NODE
{
    enum {DECL, TYPE, VAR, VARDECL, VWRITE, VSET, READ, FULLREAD, WRITE, FULLWRITE, VAL, FULLDECL} enums;
    uint32_t type;
    union
    {
        typeNODE tnode;
        varNODE vnode;
        declNODE dnode;
        vwriteNODE vwnode;
        valNODE vanode;
    };
    NODE() {}
    ~NODE() {};
};

std::deque<std::pair<std::string, uint32_t>> stdtypes =
{
    {"float", typeNODE::NUMBER},
    {"string", typeNODE::STRING}
};

std::unordered_map<std::string, uint32_t> usrtypes(stdtypes.begin(), stdtypes.end());

std::deque<NODE> NodeExpressions;
std::deque<varNODE*> ReadStack;
std::deque<uint32_t> VariableOperations;
std::unordered_map<std::string, varNODE> usrvars;

static FILE* fi = nullptr;

static int lastchar = 0;
static std::string identstr = "";
static std::string identnum = "";

static int ParseNonSpace()
{
    static int laststate = 0;
    if (lastchar == 0)
    {
        lastchar = getc(fi);
    }
    identstr = "";
    int type = -1;
    while(isspace(lastchar))
    {
        lastchar = getc(fi);
    }

    if (lastchar != EOF)
    {
        do
        {
            
            if (type == -1 && isalpha(lastchar))
                type = PWORD;
            else if (type == -1 && (isdigit(lastchar) || lastchar == '.' || (lastchar == '-' && laststate != PNUM)))
            {
                type = PNUM;
                identstr += (char)lastchar;
                lastchar = getc(fi);
            } 
            else if (type == -1 && lastchar == '\"')
            {
                identstr += '\"';
                return POTHER;
            }
            else if (type == -1 && lastchar == '#')
            {
                identstr += '#';
                lastchar = getc(fi);
                if (lastchar == '*')
                    return PRCOMMENT;
                return PCOMMENT;
            }
            else if (type == -1)
                type = POTHER;
            //printf("Char = %c, type = %i\n", (char)lastchar, type);
            if (type == PWORD && !isalnum(lastchar))
            {
                #if !defined(NPRINT)
                //printf("Cuttoff of alpha-numeric word\n");
                #endif
                break;
            }
            if (type == PNUM && !(isdigit(lastchar) || lastchar == '.') || (lastchar == '-' && (type == PNUM)))
            {
                #if !defined(NPRINT)
                //printf("Cuttoff of digit word\n");
                #endif
                break;
            }
            if (type == POTHER && isalnum(lastchar))
            {
                #if !defined(NPRINT)
                //printf("Cuttoff of unknown identity\n");
                #endif
                break;
            }
            
            identstr += (char)lastchar;
        } while(!isspace((lastchar = getc(fi))) && lastchar != EOF);
    }
    if (lastchar == EOF)
        type = -1;
    #if !defined(NPRINT)
    //printf("%s : %i\n", identstr.c_str(), type);
    #endif
    laststate = type;
    return type;
}

static int parse()
{
    
    //static int lastchar = 0;
    static int laststate = 0;
    //-- NEW WHITESPACE BASED PARSING SYSTEM --//
    switch(ParseNonSpace())
    {
        case -1:
            return EOF;
        
        case PCOMMENT:
        {
            while(lastchar != '\n' && lastchar != EOF)
            {
                lastchar = getc(fi);
                //printf("Char: %c\n", (char)lastchar);
            }
                
        }
        break;
        case PRCOMMENT:
        {
            std::string str = "";
            #if !defined(NPRINT)
            std::string commstr = "";
            #endif

            while(strcmp(str.c_str(), "*#") != 0)
            {
                if (str.size() > 1)
                {
                    str.clear();
                    str += lastchar;
                }
                lastchar = getc(fi);
                if (lastchar == EOF)
                {
                    printf("WARNING: Range based comment extends to EOF\n");
                    break;
                }
                str += lastchar;
                #if !defined(NPRINT)
                commstr += lastchar;
                #endif
                //printf("Comment string: %s\n", str.c_str());
            }
            
            #if !defined(NPRINT)
            commstr.erase(commstr.size()-2);
            printf("STATUS: Commented string: \"%s\"\n", commstr.c_str());
            #endif
        }
        break;

        case POTHER:
        {
            if (strcmp(identstr.c_str(), "=") == 0)
            {
                NODE vwnode;
                vwnode.type = NODE::VWRITE;
                vwnode.vwnode = {};
                NodeExpressions.push_back(vwnode);
                VariableOperations.push_front(NODE::VWRITE);
                return NODE::VWRITE;
            }
            //-- STRING LITERALLS --//
            if (strcmp(identstr.c_str(), "\"") == 0)
            {
                identstr = "";
                lastchar = getc(fi);
                do
                {
                    identstr += lastchar;
                    lastchar = getc(fi);
                } while(lastchar != '\"' && lastchar != EOF);
                if (lastchar == EOF)
                {
                    printf("ERROR: String literal extends to EOF\n");
                    return EOF;
                }
                if (lastchar != EOF && VariableOperations.size() != 0 && VariableOperations.at(0) == NODE::VWRITE)
                {
                    valNODE vanode;
                    vanode.setval(identstr.c_str());
                    NODE vaNODE;
                    vaNODE.type = NODE::VAL;
                    vaNODE.vanode = vanode;
                    NodeExpressions.push_back(vaNODE);
                    VariableOperations.pop_front();
                    lastchar = getc(fi);
                    #if !defined(NPRINT)
                    printf("String literal: \"%s\"\n", identstr.c_str());
                    #endif
                    return NODE::VAL;
                }
            }
        }
        break;

        case PWORD:
        {
            //-- KEYWORD SEARCH --//
            if (identstr == "new")
            {
                declNODE dnode;
                NODE dNODE;
                dNODE.type = NODE::DECL;
                dNODE.dnode = dnode;
                NodeExpressions.push_back(dNODE);
                VariableOperations.push_front(NODE::DECL);
                return NODE::DECL;
            }
            if (identstr == "set")
            {
                NODE snode;
                snode.type = NODE::VSET;
                NodeExpressions.push_back(snode);
                VariableOperations.push_front(NODE::VSET);
                return NODE::VSET;
            }
            //-- TYPENAME SEARCH --//
            {
                int ret = 0;
                auto itr = usrtypes.find(identstr);
                //printf("Iterator first: %s\n", itr->first.c_str());
                if (itr != usrtypes.end())
                {
                    
                    typeNODE tnode;
                    tnode.type = itr->second;
                    NODE tNODE;
                    tNODE.type = NODE::TYPE;
                    tNODE.tnode = tnode;
                    if (VariableOperations.size() != 0 && VariableOperations[0] == NODE::DECL)
                    {
                        NodeExpressions.push_back(tNODE);
                        VariableOperations.pop_front();
                        VariableOperations.push_front(NODE::FULLDECL);
                    }
                    ret = NODE::TYPE;
                } else
                {
                    if (VariableOperations.size() != 0 && VariableOperations[0] == NODE::DECL)
                    {
                        printf("ERROR: Expected type, but type is invalid\n");
                        VariableOperations.pop_front();
                        NodeExpressions.pop_back();
                        //return INTYPE;
                    }
                }
                if (ret != 0)
                    return ret;
            }
            //-- VARIABLE STUFF --//
            if (VariableOperations.size() != 0)
            switch(VariableOperations.at(0))
            {
                case NODE::VSET:
                {
                    varNODE vnode;
                    vnode.name = (char*)calloc(identstr.size(), 1);
                    strcpy(vnode.name, identstr.c_str());
                    NODE vNODE;
                    vNODE.type = NODE::VAR;
                    vNODE.vnode = vnode;
                    NodeExpressions.push_back(vNODE);
                    VariableOperations.pop_front();
                    return NODE::VAR;
                }
                break;
                case NODE::FULLDECL:
                {
                    if (usrvars.find(identstr) == usrvars.end())
                    {
                        varNODE vnode;
                        vnode.name = (char*)calloc(identstr.size(), 1);
                        strcpy(vnode.name, identstr.c_str());
                        NODE vNODE;
                        vNODE.type = NODE::VARDECL;
                        vNODE.vnode = vnode;
                        NodeExpressions.push_back(vNODE);
                        VariableOperations.pop_front();
                        return NODE::VARDECL;
                    }
                    printf("ERROR: invalid variable exception\n");
                    return INVAR;
                }
                break;
            }
        }
        break;

        case PNUM:
        {
            if (VariableOperations.size() != 0)
            switch(VariableOperations.at(0))
            {
                case NODE::VWRITE:
                {
                    valNODE vanode;
                    vanode.setval(identstr.c_str());
                    NODE vaNODE;
                    vaNODE.type = NODE::VAL;
                    vaNODE.vanode = vanode;
                    NodeExpressions.push_back(vaNODE);
                    VariableOperations.pop_front();
                    return NODE::VAL;
                }
                break;
                case NODE::FULLDECL:
                {
                    if (usrvars.find(identstr) == usrvars.end())
                    {
                        varNODE vnode;
                        vnode.name = (char*)calloc(identstr.size(), 1);
                        strcpy(vnode.name, identstr.c_str());
                        NODE vNODE;
                        vNODE.type = NODE::VARDECL;
                        vNODE.vnode = vnode;
                        NodeExpressions.push_back(vNODE);
                        VariableOperations.pop_front();
                        return NODE::VARDECL;
                    }
                    printf("ERROR: invalid variable exception\n");
                    return INVAR;
                }
                break;
                
            }
        }
        break;
    }
    if (lastchar == EOF)
    {
        //return EOF;
    }
    //lastchar = getc(fi);
    return 0;
}

varNODE setvar = {};

void ParseNodeExpressions()
{
    #if !defined(NPRINT)
    printf("Node count: %zu\n", NodeExpressions.size());
    #endif

    for (size_t i = 0; i < NodeExpressions.size(); ++i)
    {
        auto type = NodeExpressions[i].type;
        switch(type)
        {
            case NODE::DECL:
            {
                #if !defined(NPRINT)
                printf("Declaration expression\n");
                #endif
                setvar = {};
                //ReadStack.push_back({});
            }
            break;

            case NODE::VSET:
            {
                #if !defined(NPRINT)
                printf("Variable set expression\n");
                #endif
                setvar = {};
            }
            break;

            case NODE::TYPE:
            {
                setvar.value = NodeExpressions[i].tnode;
                //ReadStack[ReadStack.size()-1].value = NodeExpressions[i].tnode;
                #if !defined(NPRINT)
                printf("Type expression with type: %i\n", NodeExpressions[i].tnode.type);
                #endif
            }
            break;

            case NODE::VAR:
            {
                auto itr = usrvars.find(NodeExpressions[i].vnode.name);
                if (itr != usrvars.end()) // Is a declaration
                {
                    setvar = NodeExpressions[i].vnode;
                    #if !defined(NPRINT)
                    printf("Intermidiate var set to working variable \"%s\"\n", NodeExpressions[i].vnode.name);
                    #endif
                } else
                {
                    printf("ERROR: Variable name does not exist\n");
                }
            }
            break;

            case NODE::VARDECL:
            {
                setvar.name = (char*)calloc(strlen(NodeExpressions[i].vnode.name),1);
                strcpy(setvar.name, NodeExpressions[i].vnode.name);
                usrvars.emplace(setvar.name, setvar);
                //usrvars[NodeExpressions[i].vnode.name] = setvar;
                //ReadStack[ReadStack.size()-1].name = (char*)calloc(strlen(NodeExpressions[i].vnode.name),1);
                //strcpy(ReadStack[ReadStack.size()-1].name, NodeExpressions[i].vnode.name);
                #if !defined(NPRINT)
                printf("Variable created with name: %s\n", NodeExpressions[i].vnode.name);
                #endif
            }
            break;

            case NODE::VWRITE:
            {
                if(setvar.name != NULL)
                {
                    auto itr = usrvars.find(setvar.name);
                    if (itr == usrvars.end())
                    {
                        printf("ERROR: Variable write failed\n");
                        break;
                    }
                    #if !defined(NPRINT)
                    printf("Latest variable pushed to readstack for a set operation\n");
                    #endif
                    ReadStack.push_front(&itr->second);
                }
            }
            break;
            
            case NODE::VAL:
            {
                if (ReadStack.size() != 0)
                {
                    auto node = NodeExpressions[i].vanode;
                    auto ptr = ReadStack[0];
                    switch(ptr->value.type)
                    {
                        case typeNODE::NUMBER:
                        {
                            ptr->value.setnumber(node.val);
                            #if !defined(NPRINT)
                            if (ptr->name != NULL)
                                printf("Set latest readstack variable \"%s\", to float: %.2Lf\n", ptr->name, ptr->value.number.val);
                            #endif
                            ReadStack.pop_front();
                            //VariableOperations.clear();
                        }
                        break;

                        case typeNODE::STRING:
                        {
                            ptr->value.setstr(node.val);
                            #if !defined(NPRINT)
                            if (ptr->name != NULL)
                                printf("Set latest readstack variable \"%s\", to string: \"%s\"\n", ptr->name, ptr->value.str);
                            #endif
                            ReadStack.pop_front();
                        }
                        break;
                    }
                }
            }
            break;

        }
    }
}

int main(int argc, char* argv[])
{
    fi = fopen("script.vtex", "r");
    if (fi == NULL)
    {
        printf("Script wasnt found. Make a file named \"script.vtex\" in the build directory\n");
        return -1;
    }
    auto strt = c::high_resolution_clock::now();
    while(true)
    {
        if (parse() < 0)
        {
            break;
        }
    }
    

    ParseNodeExpressions();

    auto end = c::high_resolution_clock::now();
    printf("Time: %.2lfus\n", c::duration<float, c::microseconds::period>(end - strt).count());
    fclose(fi);
    return 0;
}