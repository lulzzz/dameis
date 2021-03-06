/*
 * =====================================================================================
 *
 *       Filename:  nEvalPol.c
 *
 *    Description:  This program evaluates a N-echelon base stock policy.
 *
 *        Version:  1.0
 *        Created:  2015年02月04日 16时27分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  LU Tianshu (), tssslu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>

#define MAX_PERIOD		100
#define MAX_X			150
#define MAX_N			5
#define MAX_D_LENGTH		30
#define INF			99999999
#define MAX(x,y)		((x>y)?x:y)
#define MIN(x,y)		((x<y)?x:y)

typedef union tree_node {
	double value;
	int Plc;
	union tree_node * next;
} tree_node;

//Value Function
tree_node V[MAX_PERIOD];

//Policy
tree_node Plc[MAX_N];

//Number of Echelons
int N;

//Incremental Holding Cost
double h[MAX_N];

//Penalty Cost
double p;

//Demand Distribution and Expected Demand
int D_len;
int D[MAX_D_LENGTH];
double P[MAX_D_LENGTH];
double ED = 0;

//Capacity Constraint
int K[MAX_N];

//Base stock policy
int s[MAX_N];

//Discount Factor
double beta;

//Lower & Upper Bound of X, Number of Periods
int LB, UB, period;

void init_V(tree_node * arr, int level, int prd)
{
	//TODO: This function initializes the value function. The value function is
	//	an array implemented by a tree.

	int i;
	if (level == 0) {
		for (i = 0; i <= period; i++) {
			arr[i].next = (tree_node *) calloc(MAX_X, sizeof(tree_node));
			init_V(arr[i].next, level + 1, i);
		}
	}
	else if (level == N) {
		for (i = 0; i < MAX_X; i++) {
			arr[i].value = (prd==0)?0:INF;
		}
	}
	else {
		for (i = 0; i < MAX_X; i++) {
			arr[i].next = (tree_node *) calloc(MAX_X, sizeof(tree_node));
			init_V(arr[i].next, level + 1, prd);
		}
	}
}

void init_Plc(tree_node * arr, int level)
{
	//TODO: This function initializes the value function. The value function is
	//	an array implemented by a tree.

	int i;
	if (level == 0) {
		for (i = 0; i < N; i++) {
			arr[i].next = (tree_node *) calloc(MAX_X, sizeof(tree_node));
			init_Plc(arr[i].next, level + 1);
		}
	}
	else if (level == N) {
		return;
	}
	else {
		for (i = 0; i < MAX_X; i++) {
			arr[i].next = (tree_node *) calloc(MAX_X, sizeof(tree_node));
			init_Plc(arr[i].next, level + 1);
		}
	}
}

void init()
{
	//TODO: This function initialize all variables including the value
	//	function array.

	int i;
	FILE * fp = fopen("nEchelon.dat", "r");
	fscanf(fp, "%lf%d%d", &beta, &N, &period);
	for (i = 0; i < N; i++) {
		fscanf(fp, "%lf", &h[i]);
	}
	for (i = 0; i < N; i++) {
		fscanf(fp, "%d", &K[i]);
	}
	fscanf(fp, "%lf%d%d%d", &p, &UB, &LB, &D_len);
	for (i = 0; i < D_len; i++) {
		fscanf(fp, "%d", &D[i]);
	}
	for (i = 0; i < D_len; i++) {
		fscanf(fp, "%lf", &P[i]);
		ED += D[i] * P[i];
	}
	init_V(V, 0, 0);
	init_Plc(Plc, 0);
}

double get_value(int prd, int X[])
{
	//TODO:	This function gets value from the value function.

	int lvl = 0;
	tree_node * curr = & V[prd];
	while (lvl < N) {
		curr = curr -> next;
		curr += (X[lvl++] + MAX_X/2);
	}
	return curr->value;
}

void set_value(int prd, int X[], double tgt)
{
	//TODO: This function sets value in the value function.

	int lvl = 0;
	tree_node * curr = & V[prd];
	while (lvl < N) {
		curr = curr -> next;
		curr += (X[lvl++] + MAX_X/2);
	}
	curr -> value = tgt;
}

double L(int Y[])
{
	//TODO: Given a decison Y[N], this function tells its holding 
	//	and penalty costs.
	//VAR:	EDYp		E[max(D-Y,0)]
	//	pen_hol		p+h_1+h_2+...+h_N

	int i;
	double res = 0, ED = 0, pen_hol = p, EDYp = 0;
	for (i = 0; i < D_len; i++) {
		EDYp += MAX(D[i]-Y[0], 0) * P[i];
	}
	for (i = 0; i < N; i++) {
		res += h[i] * (Y[i] - ED);
		pen_hol += h[i];
	}
	res += pen_hol * EDYp;
	return res;
}

double J(int prd, int Y[])
{
	//TODO: Given a decision Y[N] and the current period, this
	//	function tells the value of objective function.
	//VAR:	EV		E[V_n-1(Y-D)]
	//	tmpY		tmpY = Y - D

	int i, j, tmpY[MAX_N];
	double res, EV = 0;
	res = L(Y);
	for (i = 0; i < D_len; i++) {
		for (j = 0; j < N; j++) {
			tmpY[j] = Y[j] - D[i];
		}
		EV += P[i] * get_value(prd - 1, tmpY);
	}
	res += beta * EV;
	return res;
}

void set_Y(int X[], int Y[])
{
	int i, tmpY;
	for (i = 0; i < N - 1; i++) {
		tmpY = MIN(X[i]+K[i], X[i+1]);
		tmpY = MIN(tmpY, s[i]);
		Y[i] = MAX(X[i], tmpY);
	}
	tmpY = MIN(s[i], X[N-1]+K[N-1]);
	Y[N-1] = MAX(tmpY, X[N-1]);
}

void DP(int prd, int X[])
{
	//TODO:	This function receives a set of state variables and then
	//	iterates the value function for one step.

	int Y[MAX_N];
	set_Y(X, Y);
	set_value(prd, X, J(prd, Y));
}

void perm_X(int prd, int idx, int res[])
{
	//TODO:	This function permutates among all possible state
	//	variables.

	if (idx == 0) {
		for (res[idx] = LB; res[idx] <= UB; res[idx]++) {
			perm_X(prd, 1, res);
		}
	}
	else if (idx != N - 1) {
		for (res[idx] = res[idx-1]; res[idx] <= UB; res[idx]++) {
			perm_X(prd, idx+1, res);
		}
	}
	else {
		for (res[idx] = res[idx-1]; res[idx] <= UB; res[idx]++) {
			DP(prd, res);
		}
	}
	return;
}

void show_result(int X[])
{
	//TODO:	This function shows the optimal policy for a given 
	//	set of initial installation inventory.

	int i;
	for (i = 0; i < N; i++) {
		printf("%d\t", (i==0)?X[0]:(X[i]-X[i-1]));
	}
	for (i = 0; i < N; i++) {
		printf("%d\t", X[i]);
	}
	printf("%.2lf\n", get_value(period, X));
}

int main(int argc, const char *argv[])
{
	int i, flag = 1, tmpX[MAX_X],
	    X[MAX_N], x[MAX_N];
	init();

	for (i = 0; i < N; i++) {
		scanf("%d", &s[i]);
	}

	//TODO:	Iterate through periods
	for (i = 1; i <= period; i++) {
		perm_X(i, 0, tmpX);
	}

	//TODO: Read installation inventories and print the optimal policy
	while (flag != EOF) {
		for (i = 0; i < N; i++) {
			flag = scanf("%d", &x[i]);
			if (i == 0) {
				X[0] = x[0];
			}
			else {
				X[i] = X[i-1]+x[i];
			}
		}
		if (flag != EOF) {
			show_result(X);
		}
	}
	return 0;
}
