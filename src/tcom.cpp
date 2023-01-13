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

struct typeNODE
{
    enum {NUMBER, STRING} enums;
    uint32_t type;
    union
    {
        long double number;
        char* str;
    };
    void setnumber(const std::string val) {number = strtold(val.c_str(), nullptr);}
    void setstr(const std::string val) {str = const_cast<char*>(val.c_str());}
    typeNODE() {}
    ~typeNODE() {}
};

struct varNODE
{
    char* name;
    typeNODE value;
    varNODE() {}
    ~varNODE() {}
};

struct declNODE
{
    declNODE() {}
    ~declNODE() {}
};

struct NODE
{
    enum {DECL, TYPE, VAR, VWRITE, VAL} enums;
    uint32_t type;
    union
    {
        typeNODE tnode;
        varNODE vnode;
        declNODE dnode;
    };
    NODE() {}
    ~NODE() {};
};

std::deque<NODE> NodeExpressions;

int main(int argc, char* argv[])
{
    auto strt = c::high_resolution_clock::now();
    {
        declNODE dnode;
        NODE dNODE;
        dNODE.type = NODE::DECL;
        dNODE.dnode = dnode;
        NodeExpressions.push_back(dNODE);

        typeNODE tnode;
        tnode.type = typeNODE::NUMBER;
        tnode.setnumber("69.420");
        //tnode.number = 69.4;
        NODE tNODE;
        tNODE.type = NODE::TYPE;
        tNODE.tnode = tnode;
        NodeExpressions.push_back(tNODE);

        varNODE vnode;
        vnode.value = NodeExpressions[0].tnode;
        vnode.name = new char[strlen("hello")];
        strcpy(vnode.name, "hello");
        NODE vNODE;
        vNODE.type = NODE::VAR;
        vNODE.vnode = vnode;
        NodeExpressions.push_back(vNODE);
    }

    for (size_t i = 0; i < NodeExpressions.size(); ++i)
    {
        auto type = NodeExpressions[i].type;
        switch(type)
        {
            case NODE::DECL:
            {
                //printf("Declaration expression\n");
            }
            break;

            case NODE::TYPE:
            {
                //printf("Type expression with type: %i\n", NodeExpressions[i].tnode.type);
            }
            break;

            case NODE::VAR:
            {
                //printf("Variable expression with name: %s\n", NodeExpressions[i].vnode.name);
            }
            break;
        }
    }
    auto end = c::high_resolution_clock::now();
    printf("Time: %.2lfus\n", c::duration<float, c::microseconds::period>(end - strt).count());
    return 0;
}
