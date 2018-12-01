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

const static int THIS_NODE = 0;

int connectcosts0[4] = { 0,  1,  3, 7 };

struct distance_table 
{
  int costs[4][4];
} dt0;

/* students to write the following two routines, and maybe some others */

/* This routine will be called once at the beginning of the emulation.
 * rtinit0() has no arguments. It should initialize the distance table in node 0
 * to reflect the direct costs of 1, 3, and 7 to nodes 1, 2, and 3,
 * respectively. In Figure 1, all links are bi-directional and the costs in both
 * directions are identical. After initializing the distance table, and any
 * other data structures needed by your node 0 routines, it should then send its
 * directly-connected neighbors (in this case, 1, 2 and 3) the cost of it
 * minimum cost paths to all other network nodes. This minimum cost information
 * is sent to neighboring nodes in a routing packet by calling the routine
 * tolayer2(), as described below. The format of the routing packet is also
 * described below.
 */
void rtinit0() 
{
  /* Print called at time */
  printf("rtinit0 called at %f\n", clocktime);

  /* Initialize all costs to infinity. We'll update directly connected nodes
   * next. */
  for (int fromnode = 0; fromnode <= 3; ++fromnode) {
    for (int tonode = 0; tonode <= 3; ++tonode) {
      dt0.costs[fromnode][tonode] = 999;
    }
  }

  /* Initialize direct costs of 1, 3, and 7 to nodes 1, 2, and 3 */
  dt0.costs[THIS_NODE][THIS_NODE] = 0;
  dt0.costs[1][1] = 1;
  dt0.costs[2][2] = 3;
  dt0.costs[3][3] = 7;

  informneighbors0();

}

/* This routine will be called when node 0 receives a routing packet that was
 * sent to it by one if its directly connected neighbors. The parameter *rcvdpkt
 * is a pointer to the packet that was received.
 *
 * rtupdate0() is the ``heart'' of the distance vector algorithm. The values it
 * receives in a routing packet from some other node i contain i's current
 * shortest path costs to all other network nodes. rtupdate0() uses these
 * received values to update its own distance table (as specified by the
 * distance vector algorithm). If its own minimum cost to another node changes
 * as a result of the update, node 0 informs its directly connected neighbors of
 * this change in minimum cost by sending them a routing packet. Recall that in
 * the distance vector algorithm, only directly connected nodes will exchange
 * routing packets. Thus nodes 1 and 2 will communicate with each other, but
 * nodes 1 and 3 will node communicate with each other.
 */
void rtupdate0(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
  /* Print called at time */
  printf("rtupdate0 called at %f\n", clocktime);

  int neighbor = rcvdpkt->sourceid;

  /* Print sender */
  printf("Sender of routing packet: %d\n", neighbor);

  /* Did the distance table change? */
  int isdistancechanged = 0;

  /* See if we need to update our costs to each possible destination */
  for (int dest = 0; dest <= 3; ++dest) {

    /* Cost from me to destination via neighbor = cost from me to neighbor + cost from neighbor to destination */
    int newcost = connectcosts0[neighbor] + rcvdpkt->mincost[dest];

    /* Update my route via this neighbor iff the neighbor found a better route.
     * OR, if the neighbor is currently my best route but they increased the route cost (this handles link changes) */
    if (newcost < dt0.costs[dest][neighbor] ||
        (findmincost0(dest) == neighbor && newcost > dt0.costs[dest][neighbor])) {

      printf("Distance table was updated for route %d to %d. "
             "Cost changed: %d -> %d via %d. We will inform the neighbors.\n",
             THIS_NODE, dest,
             dt0.costs[dest][neighbor], newcost, neighbor);

      /* It's cheaper to go via the neighbor, so update our cost */
      dt0.costs[dest][neighbor] = newcost;

      /* Set the flag to indicate that the distance table was changed */
      isdistancechanged = 1;
    }
  }

  /* If we updated our distance table costs, inform the neighbors so they can
   * update their costs */
  if (isdistancechanged == 1) {
    informneighbors0();
  } else {
    printf("Distance table was not updated.\n");
  }

  /* Print the distance table */
  printdt0(&dt0);

}


printdt0(dtptr)
  struct distance_table *dtptr;
  
{
  printf("                via     \n");
  printf("   D0 |    1     2    3 \n");
  printf("  ----|-----------------\n");
  printf("     1|  %3d   %3d   %3d\n",dtptr->costs[1][1],
	 dtptr->costs[1][2],dtptr->costs[1][3]);
  printf("dest 2|  %3d   %3d   %3d\n",dtptr->costs[2][1],
	 dtptr->costs[2][2],dtptr->costs[2][3]);
  printf("     3|  %3d   %3d   %3d\n",dtptr->costs[3][1],
	 dtptr->costs[3][2],dtptr->costs[3][3]);
}

linkhandler0(linkid, newcost)   
  int linkid, newcost;

/* called when cost from 0 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
	
{
  /* Print called at time */
  printf("linkhandler0 called at %f\n", clocktime);

  /* Print debugging info */
  printf("Link cost was updated for link %d to %d. "
       "Cost changed: %d -> %d. We will inform the neighbors.\n",
       THIS_NODE, linkid,
       dt0.costs[linkid][linkid], newcost);

  /* Update link cost and inform neighbors */
  connectcosts0[linkid] = newcost;
  dt0.costs[linkid][linkid] = newcost;

  informneighbors0();

  /* Print the table for debugging */
  printdt0(&dt0);
}

/* Return the neighbor with the cheapest cost from me to destination. */
int findmincost0(int destnode)
{
  int minvia = -1;
  int min = 999;
  for (int i = 0; i <=3; ++i) {
    if (dt0.costs[destnode][i] < min) {
      minvia = i;
      min = dt0.costs[destnode][i];
    }
  }
  return minvia;
}

/* Inform neighbors that the distance vector from this node has changed */
informneighbors0()
{
  struct rtpkt p;

  /* Calculate the minimum costs to each destination for p.mincost[] */
  for (int dest = 0; dest <= 3; ++dest) {
    int mincostvia = findmincost0(dest);
    p.mincost[dest] = dt0.costs[dest][mincostvia];
  }

  /* Inform all the neighbors */
  p.sourceid = THIS_NODE;
  for (int neighbor = 0; neighbor <=3; ++neighbor) {
    if (neighbor == THIS_NODE) continue;
    p.destid = neighbor;
    tolayer2(p);
  }
}
