// C HEADINGS
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
// CXX HEADINGS
#include <string>
#include <unordered_map>
#include <deque>

#include <chrono>
namespace c = std::chrono;

struct valNODE
{
    char* val;
    void setval(const char* _val) {val = (char*)calloc(strlen(_val), 1); strcpy(val, _val);}
};

struct typeNODE
{
    enum {NUMBER, STRING} enums;
    uint32_t type;
    union
    {
        long double number;
        char* str;
    };
    void setnumber(const char* val) {number = strtold(val, nullptr);}
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
    vwriteNODE() {}
    ~vwriteNODE() {}
};

struct NODE
{
    enum {DECL, TYPE, VAR, VWRITE, VAL, FULLDECL} enums;
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
std::deque<varNODE> ReadStack;
std::deque<uint32_t> VariableOperations;

static FILE* fi = nullptr;

static std::string identstr = "";
static std::string identnum = "";

static int parse()
{
    
    static int lastchar = 0;
    static int laststate = 0;

    while(isspace(lastchar))
    {
        lastchar = getc(fi);
    }

    if (lastchar == '=')
    {
        lastchar = getc(fi);
        VariableOperations.push_front(NODE::VWRITE);
        return NODE::VWRITE;
    }
    if (lastchar == '$')
    {
        //lastchar = getc(fi);
        //return vread;
    }
    if (lastchar == '\"')
    {
        identstr = "";
        lastchar = getc(fi);
        do
        {
            identstr += lastchar;
            lastchar = getc(fi);
        } while(lastchar != '\"');
        if (VariableOperations.size() != 0 && VariableOperations.at(0) == NODE::VWRITE)
        {
            valNODE vanode;
            vanode.setval(identstr.c_str());
            NODE vaNODE;
            vaNODE.type = NODE::VAL;
            vaNODE.vanode = vanode;
            NodeExpressions.push_back(vaNODE);
            VariableOperations.erase(VariableOperations.begin());
            lastchar = getc(fi);
            return NODE::VAL;
        }
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
                declNODE dnode;
                NODE dNODE;
                dNODE.type = NODE::DECL;
                dNODE.dnode = dnode;
                NodeExpressions.push_back(dNODE);
                VariableOperations.push_front(NODE::DECL);
                return NODE::DECL;
            }
        }
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
                    VariableOperations.erase(VariableOperations.begin());
                    VariableOperations.push_front(NODE::FULLDECL);
                }
                ret = NODE::TYPE;
            } else
            {
                if (VariableOperations.size() != 0 && VariableOperations[0] == NODE::DECL)
                {
                    VariableOperations.erase(VariableOperations.begin());
                }
            }
            if (ret != 0)
                return ret;
        }
        
        if (VariableOperations.size() != 0)
        switch(VariableOperations.at(0))
        {
            case NODE::FULLDECL:
            {
                varNODE vnode;
                vnode.value = NodeExpressions[0].tnode;
                vnode.name = (char*)calloc(identstr.size(), 1);
                strcpy(vnode.name, identstr.c_str());
                NODE vNODE;
                vNODE.type = NODE::VAR;
                vNODE.vnode = vnode;
                NodeExpressions.push_back(vNODE);
                VariableOperations.erase(VariableOperations.begin());
                return NODE::VAR;
            }
            break;
        }
    }

    if (isdigit(lastchar) || lastchar == '.')
    {
        identnum = "";
        do
        {
            identnum += (char)lastchar;
        } while(isdigit((lastchar = getc(fi))) || lastchar == '.');
        if (VariableOperations.size() != 0)
        switch(VariableOperations.at(0))
        {
            case NODE::VWRITE:
            {
                valNODE vanode;
                vanode.setval(identnum.c_str());
                NODE vaNODE;
                vaNODE.type = NODE::VAL;
                vaNODE.vanode = vanode;
                NodeExpressions.push_back(vaNODE);
                VariableOperations.erase(VariableOperations.begin());
                lastchar = getc(fi);
                return NODE::VAL;
            }
            break;
            
        }
    }
    if (lastchar == EOF)
    {
        return EOF;
    }
    lastchar = getc(fi);
    return 0;
}

int main(int argc, char* argv[])
{
    fi = fopen("script.vtex", "r");
    if (fi == NULL)
    {
        return -1;
    }
    while(true)
    {
        if (parse() < 0)
        {
            break;
        }
    }
    auto strt = c::high_resolution_clock::now();
    for (size_t i = 0; i < NodeExpressions.size(); ++i)
    {
        auto type = NodeExpressions[i].type;
        switch(type)
        {
            case NODE::DECL:
            {
                printf("Declaration expression\n");
                ReadStack.push_back({});
            }
            break;

            case NODE::TYPE:
            {
                ReadStack[ReadStack.size()-1].value = NodeExpressions[i].tnode;
                printf("Type expression with type: %i\n", NodeExpressions[i].tnode.type);
            }
            break;

            case NODE::VAR:
            {
                ReadStack[ReadStack.size()-1].name = (char*)calloc(strlen(NodeExpressions[i].vnode.name),1);
                strcpy(ReadStack[ReadStack.size()-1].name, NodeExpressions[i].vnode.name);
                printf("Variable expression with name: %s\n", NodeExpressions[i].vnode.name);
            }
            break;

            case NODE::VAL:
            {
                auto node = NodeExpressions[i].vanode;
                auto ptr = &ReadStack[ReadStack.size()-1];
                switch(ptr->value.type)
                {
                    case typeNODE::NUMBER:
                    {
                        if (ptr->name != NULL)
                            printf("Set latest readstack variable \"%s\", to float: %s\n", ptr->name, node.val);
                        ptr->value.setnumber(node.val);
                        ReadStack.erase(ReadStack.end()-1);
                        VariableOperations.clear();
                    }
                    break;

                    case typeNODE::STRING:
                    {
                        printf("Set latest readstack variable to string: \"%s\"\n", node.val);
                        ptr->value.setstr(node.val);
                        ReadStack.erase(ReadStack.end()-1);
                    }
                    break;
                }
            }
            break;

        }
    }
    auto end = c::high_resolution_clock::now();
    printf("Time: %.2lfus\n", c::duration<float, c::microseconds::period>(end - strt).count());
    fclose(fi);
    return 0;
}
