#include <stdio.h>

typedef struct process
{
    int id;
    int crash;
} process;
process P[10];

int total, coordinator;

int highest()
{
    int max = 0, i, loc;

    for (i = 1; i <= total; i++)
    {
        if (P[i].id > max)
        {
            if (P[i].crash == 0)
            {
                max = P[i].id;
                loc = i;
            }
        }
    }
    return (loc);
}

void election(int newCoord)
{
    int i, j, new;
    int total1 = 0;
    for (j = 1; j <= total; j++)
        if (P[j].crash != 1)
            total1++;
    while (newCoord <= total1)
    {
    for(i=newCoord+1;i<=total;i++)
     if(P[newCoord].id< P[i].id)
        printf("\n  Election message sent to Process %d by process %d \n",i,newCoord);
    printf("\n");
    for(i=newCoord+1;i<=total;i++)
     if(P[i].crash==0 && P[newCoord].id< P[i].id )
         printf("\n  OK message received from Process %d \n",i);
   else if(P[i].crash=1 && P[newCoord].id< P[i].id)
             printf("\n  Process %d is not responding.. \n",i);
        newCoord = newCoord + 1;
        if (newCoord <= total1)
            printf("\n  process %d is taking over.. \n", newCoord);
    }
    coordinator = newCoord - 1;
    printf("\n New elected coordinator is Process %d", coordinator);
}

void Crash()
{
    int no, i, newCoord;
    printf("\n  Enter the Process Number of the Process to be crashed:  \n");
    scanf("%d", &no);
    P[no].crash = 1;
    printf("\n  Process %d has crashed.. ", no);
    for (i = 1; i <= total; i++)
    {
        if (P[i].crash == 0)
            printf("\n Process %d is active \n", i);
        else
            printf("\n Process %d is Inactive \n", i);
    }
    if (no == coordinator)
    {
        printf("\n  Enter a process number which should start the election: ");
        scanf("%d", &newCoord);
        election(newCoord);
    }
}

void Bully()
{
    int op;
    coordinator = highest();
    printf("\n  Process %d is the Coordinator...", coordinator);
    do
    {
        printf("\nn 1.Crash \n 2.Exit ");
        printf("\n Enter your choice: ");
        scanf("%d", &op);
        switch (op)
        {
        case 1:
            Crash();
            break;
        case 2:
            break;
        }
    } while (op != 2);
}

void main()
{

    int i, id;
    int ch;
    printf("\n Enter Number of Processes: ");
    scanf("%d", &total);
    printf("\nEnter the id for the processes from low priority to high priorityn");
    for (i = 1; i <= total; i++)
    {
        printf("\n Enter id for Process %d: ", i);
        scanf("%d", &id);
        P[i].id = id;
        P[i].crash = 0;
    }
    Bully();
}
