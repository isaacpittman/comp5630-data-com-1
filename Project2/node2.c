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

const static int THIS_NODE = 2;

int connectcosts2[4] = { 3, 1, 0, 2 };

struct distance_table 
{
  int costs[4][4];
} dt2;


/* students to write the following two routines, and maybe some others */

void rtinit2() 
{
  /* Print called at time */
  printf("rtinit2 called at %f\n", clocktime);

  /* Initialize all costs to infinity. We'll update directly connected nodes
   * next. */
  for (int fromnode = 0; fromnode <=3; ++fromnode) {
    for (int tonode = 0; tonode <= 3; ++tonode) {
      dt2.costs[fromnode][tonode] = 999;
    }
  }

  dt2.costs[THIS_NODE][THIS_NODE] = 0;
  dt2.costs[0][0] = 3;
  dt2.costs[1][1] = 1;
  dt2.costs[3][3] = 2;

  informneighbors2();

}


void rtupdate2(rcvdpkt)
  struct rtpkt *rcvdpkt;
  
{
  /* Print called at time */
  printf("rtupdate2 called at %f\n", clocktime);

  int neighbor = rcvdpkt->sourceid;

  /* Print sender */
  printf("Sender of routing packet: %d\n", neighbor);

  /* Did the distance table change? */
  int isdistancechanged = 0;

  /* See if we need to update our costs to each possible destination */
  for (int dest = 0; dest <= 3; ++dest) {

    /* Cost from me to destination via neighbor = cost from me to neighbor + cost from neighbor to destination */
    int newcost = connectcosts2[neighbor] + rcvdpkt->mincost[dest];

    /* Update my route via this neighbor iff the neighbor found a better route.
     * OR, if the neighbor is currently my best route but they increased the route cost (this handles link changes) */
    if (newcost < dt2.costs[dest][neighbor] ||
        (findmincost2(dest) == neighbor && newcost > dt2.costs[dest][neighbor])) {

      printf("Distance table was updated for route %d to %d. "
             "Cost changed: %d -> %d via %d. We will inform the neighbors.\n",
             THIS_NODE, dest,
             dt2.costs[dest][neighbor], newcost, neighbor);

      /* It's cheaper to go via the neighbor, so update our cost */
      dt2.costs[dest][neighbor] = newcost;

      /* Set the flag to indicate that the distance table was changed */
      isdistancechanged = 1;
    }
  }


  /* If we updated our distance table costs, inform the neighbors so they can
   * update their costs */
  if (isdistancechanged == 1) {
    informneighbors2();
  } else {
    printf("Distance table was not updated.\n");
  }

  /* Print the distance table */
  printdt2(&dt2);
}


printdt2(dtptr)
  struct distance_table *dtptr;
  
{
  printf("                via     \n");
  printf("   D2 |    0     1    3 \n");
  printf("  ----|-----------------\n");
  printf("     0|  %3d   %3d   %3d\n",dtptr->costs[0][0],
	 dtptr->costs[0][1],dtptr->costs[0][3]);
  printf("dest 1|  %3d   %3d   %3d\n",dtptr->costs[1][0],
	 dtptr->costs[1][1],dtptr->costs[1][3]);
  printf("     3|  %3d   %3d   %3d\n",dtptr->costs[3][0],
	 dtptr->costs[3][1],dtptr->costs[3][3]);
}

/* Return the neighbor with the cheapest cost from me to destination. */
int findmincost2(int destnode)
{
  int minvia = -1;
  int min = 999;
  for (int i = 0; i <=3; ++i) {
    if (dt2.costs[destnode][i] < min) {
      minvia = i;
      min = dt2.costs[destnode][i];
    }
  }
  return minvia;
}

/* Inform neighbors that the distance vector from this node has changed */
informneighbors2()
{
  struct rtpkt p;

  /* Calculate the minimum costs to each destination for p.mincost[] */
  for (int dest = 0; dest <= 3; ++dest) {
    int mincostvia = findmincost2(dest);
    p.mincost[dest] = dt2.costs[dest][mincostvia];
  }

  /* Inform all the neighbors */
  p.sourceid = THIS_NODE;
  for (int neighbor = 0; neighbor <=3; ++neighbor) {
    if (neighbor == THIS_NODE) continue;
    p.destid = neighbor;
    tolayer2(p);
  }
}



