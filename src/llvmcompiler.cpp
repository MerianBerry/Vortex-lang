//C includes
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>

//CXX includes
#include <memory>
#include <chrono>
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


namespace c = std::chrono;

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

    LeafArray(/* args */)
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
};

static unsigned int g_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
inline long double fastrand() { 
  g_seed = (214013*g_seed+2531011); 
  return (long double)((g_seed>>16)&0x7FFF) / (long double)0x7FFF; 
} 


enum toks
{
    tok_eof,

    tok_func,

    tok_extern,

    tok_ident,
    tok_number
};


const std::pair<const char*, int> keywords = {
    {"function", tok_func},
    {"extern", tok_extern}
};

static char identstr[1048] = "";
static double numval = 0.0;

static int gettok(FILE* file)
{
    static int lastchar = ' ';

    while (isspace(lastchar))
        lastchar = getc(file);
    

    if (isalpha(lastchar))
    {
        memset(identstr, 0, sizeof(identstr));
        while (isalnum((lastchar = getc(file))))
        {
            const char ch[1] = {(char)lastchar};
            strncat(identstr, ch, 1);
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    
}