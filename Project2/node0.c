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

const static int INF = 999;

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

  /* Initialize distance table direct costs of 1, 3, and 7 to nodes 1, 2, and 3 */
  const int INIT_ROW = 0;
  dt0.costs[INIT_ROW][0] = 0;
  dt0.costs[INIT_ROW][1] = 1;
  dt0.costs[INIT_ROW][2] = 3;
  dt0.costs[INIT_ROW][3] = 7;

  // Inform neighbors


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
}
