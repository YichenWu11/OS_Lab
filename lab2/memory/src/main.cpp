#include <iostream>
#include <Allocator.h>
#include <unistd.h>
#include <cassert>

Allocator::Option option = Allocator::Option::BF;

const int DEFAULT_MEM_SIZE = 1024;

Allocator* alloc = nullptr;

void DisplayMenu();
int Run(int num);

int main()
{
    int operation = 0;

    while (true)
    {
        DisplayMenu();
        std::cin >> operation;
        if (!Run(operation))
            break;
    }

    return 0;
}

void DisplayMenu()
{
    printf("-----------------[Menu]-----------------\n");
    printf("1 - Set memory size (default=%d)\n", DEFAULT_MEM_SIZE);
    printf("2 - Select memory allocation algorithm\n");
    printf("3 - New process \n");
    printf("4 - Terminate a process \n");
    printf("5 - Free a Part of memory of a process \n");
    printf("6 - Display memory usage \n");
    printf("0 - Exit\n");
    printf("-----------------[Menu]-----------------\n");
}

int Run(int num)
{
    static std::string ignore;
    switch (num)
    {
        case 1:
        {
            if (!alloc)
            {
                int size = 0;
                std::cout << "Enter the value of size..." << std::endl;
                std::cin >> size;
                assert(size > 0);
                alloc = new Allocator(size);
            }
            else
            {
                std::cerr << "Alloc has been Initilized!!!" << std::endl;
            }
            break;
        }
        case 2:
            {
                if (!alloc) 
                {
                    std::cerr << "Initilize the allocator first!!!" << std::endl;
                    break;
                };
                std::cout << "1: FF  2: BF  3: WF" << std::endl;
                int num = 0;
                std::cout << "Enter the Num..." << std::endl;
                std::cin >> num;
                if (num == 1) option = Allocator::Option::FF;
                else if (num == 2)  option = Allocator::Option::BF;
                else if (num == 3)  option = Allocator::Option::WF;
                else std::cerr << "Out of Range!!!" << std::endl;
                break;
            }
        case 3:
            {
                if (!alloc) 
                {
                    std::cerr << "Initilize the allocator first!!!" << std::endl;
                    break;
                };
                int size = 1;
                std::cout << "Enter the Size of the new Process..." << std::endl;
                std::cin >> size;
                int res = alloc->AddProcessAndAllocate(size, option);
                if (res == -1) std::cerr << "Allocate Failed!!!" << std::endl;
                break;        
            }
        case 4:
            {
                if (!alloc) 
                {
                    std::cerr << "Initilize the allocator first!!!" << std::endl;
                    break;
                };
                int pid = 1;
                std::cout << "Enter the Pid of the selected Process..." << std::endl;
                std::cin >> pid;
                assert(pid > 0);
                alloc->DelProcessAndFree(pid);
                break;
            }
        case 5:
            {
                if (!alloc) 
                {
                    std::cerr << "Initilize the allocator first!!!" << std::endl;
                    break;
                };
                int pid = 0;
                int offset = 0;
                int size = 0;
                std::cout << "Enter the Pid, Offset, Size in order..." << std::endl;
                std::cin >> pid >> offset >> size;
                if (pid == 0 || size == 0)
                {
                    std::cerr << "Input Error" << std::endl;
                    break;
                }
                alloc->NotDelProcessAndFree(pid, offset, size);
                break;
            }
        case 6:
            {
                if (!alloc) 
                {
                    std::cerr << "Initilize the allocator first!!!" << std::endl;
                    break;
                };
                alloc->OutputGraph();
                break;
            }
        case 0:
            return 0;   
        default:
            break;
    }
    std::cout << "Enter To Continue......" << std::endl;
    std::cin >> ignore;
    system("clear");
    return num;
}