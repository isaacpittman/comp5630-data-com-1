#include <stdio.h>

extern struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent 
                         (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
  };

extern int TRACE;
extern int YES;
extern int NO;
extern float clocktime;

const static int THIS_NODE = 3;

int connectcosts3[4] = { 7, 999, 2, 0 };

struct distance_table 
{
  int costs[4][4];
} dt3;

/* students to write the following two routines, and maybe some others */

void rtinit3() 
{
  /* Print called at time */
  printf("rtinit3 called at %f\n", clocktime);

  /* Initialize all costs to infinity. We'll update directly connected nodes
   * next. */
  for (int fromnode = 0; fromnode <=3; ++fromnode) {
    for (int tonode = 0; tonode <= 3; ++tonode) {
      dt3.costs[fromnode][tonode] = 999;
    }
  }

  /* Initialize direct costs of 1, 3, and 7 to nodes 1, 2, and 3 */
  dt3.costs[THIS_NODE][THIS_NODE] = 0;
  dt3.costs[0][0] = 7;
  dt3.costs[2][2] = 2;

  informneighbors3();
}


void rtupdate3(rcvdpkt)
  struct rtpkt *rcvdpkt;
  
{
  /* Print called at time */
  printf("rtupdate3 called at %f\n", clocktime);

  int neighbor = rcvdpkt->sourceid;

  /* Print sender */
  printf("Sender of routing packet: %d\n", neighbor);

  /* Did the distance table change? */
  int isdistancechanged = 0;

  /* See if we need to update our costs to each possible destination */
  for (int dest = 0; dest <= 3; ++dest) {

    /* Cost from me to destination via neighbor = cost from me to neighbor + cost from neighbor to destination */
    int newcost = connectcosts3[neighbor] + rcvdpkt->mincost[dest];

    /* Update my route via this neighbor iff the neighbor found a better route.
     * OR, if the neighbor is currently my best route but they increased the route cost (this handles link changes) */
    if (newcost < dt3.costs[dest][neighbor] ||
        (findmincost3(dest) == neighbor && newcost > dt3.costs[dest][neighbor])) {

      printf("Distance table was updated for route %d to %d. "
             "Cost changed: %d -> %d via %d. We will inform the neighbors.\n",
             THIS_NODE, dest,
             dt3.costs[dest][neighbor], newcost, neighbor);

      /* It's cheaper to go via the neighbor, so update our cost */
      dt3.costs[dest][neighbor] = newcost;

      /* Set the flag to indicate that the distance table was changed */
      isdistancechanged = 1;
    }
  }


  /* If we updated our distance table costs, inform the neighbors so they can
   * update their costs */
  if (isdistancechanged == 1) {
    informneighbors3();
  } else {
    printf("Distance table was not updated.\n");
  }

  /* Print the distance table */
  printdt3(&dt3);
}


printdt3(dtptr)
  struct distance_table *dtptr;
  
{
  printf("             via     \n");
  printf("   D3 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 1|  %3d   %3d\n",dtptr->costs[1][0], dtptr->costs[1][2]);
  printf("     2|  %3d   %3d\n",dtptr->costs[2][0], dtptr->costs[2][2]);

}


/* Return the neighbor with the cheapest cost from me to destination. */
int findmincost3(int destnode)
{
  int minvia = -1;
  int min = 999;
  for (int i = 0; i <=3; ++i) {
    if (dt3.costs[destnode][i] < min) {
      minvia = i;
      min = dt3.costs[destnode][i];
    }
  }
  return minvia;
}

/* Inform neighbors that the distance vector from this node has changed */
informneighbors3()
{
  struct rtpkt p;

  /* Calculate the minimum costs to each destination for p.mincost[] */
  for (int dest = 0; dest <= 3; ++dest) {
    int mincostvia = findmincost3(dest);
    p.mincost[dest] = dt3.costs[dest][mincostvia];
  }

  /* Inform all the neighbors */
  p.sourceid = THIS_NODE;
  for (int neighbor = 0; neighbor <=3; ++neighbor) {
    if (neighbor == THIS_NODE || neighbor == 1) continue;
    p.destid = neighbor;
    tolayer2(p);
  }
}
