#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "point.h"
#include "sorted_points.h"
#include <math.h>

struct point_node{

        struct point *pt;
        double origin_dist;
        struct point_node *next;

};


struct sorted_points {
	/* you can define this struct to have whatever fields you want. */
	struct point_node *head;
	

};


struct sorted_points *
sp_init()
{
	struct sorted_points *sp;

	sp = (struct sorted_points *)malloc(sizeof(struct sorted_points));
	assert(sp);
	sp->head = NULL;  
	
	return sp;
}

void
sp_destroy(struct sorted_points *sp)
{
	struct point_node *temp;
	struct point_node *temp_next;
	temp = sp->head;
	if(sp->head == NULL){
		free(sp);
		sp = NULL;
		return;
	}
	temp_next = temp->next;
	while(temp != NULL){
		free(temp->pt);
		temp->pt = NULL;
		free(temp);
		temp = NULL;
		temp = temp_next;
		if(temp!=NULL)
			temp_next = temp->next;
	}
	
	free(sp);
	sp = NULL;
}

int
sp_add_point(struct sorted_points *sp, double x, double y)
{
	struct point_node *new_point = (struct point_node *)malloc(sizeof(struct point_node));
	
	struct point *ptr = (struct point *)malloc(sizeof(struct point));
	
	new_point->pt = point_set(ptr,x,y);
	
	new_point->next = NULL;

	new_point->origin_dist = sqrt(pow(point_Y(new_point->pt),2) + pow(point_X(new_point->pt),2));
	
	struct point_node *temp;
	struct point_node *pre; 
	temp = sp->head;
	pre = NULL;
	
	if(temp == NULL){
		sp->head = new_point;
		return 1;
	}
	
	while(temp->next != NULL){
		if(temp->origin_dist <new_point->origin_dist){
			pre = temp;
			temp = temp->next;
		}else if(temp->origin_dist >new_point->origin_dist){
			if(pre == NULL)
				sp->head = new_point;
			else
				pre->next = new_point;
			new_point->next = temp;
			return 1;
		}else{ /*distance is same*/
			if(point_X(new_point->pt) < point_X(temp->pt)){
				if(pre == NULL)
					sp->head = new_point;
				else
					pre->next = new_point;
				new_point->next = temp;
				return 1;
			}
			
			if(point_X(new_point->pt) > point_X(temp->pt)){
				/*consider for special case when two points have same origin_dist*/
				while((temp->next != NULL) && new_point->origin_dist == temp->origin_dist && point_X(new_point->pt) > point_X(temp->pt)){
					pre = temp;
					temp = temp->next;
				}
				
				if(point_X(new_point->pt) != point_X(temp->pt)){
					
					if(temp->next == NULL && point_X(new_point->pt) > point_X(temp->pt)){
						temp->next = new_point;
						return 1;
					}
					pre->next = new_point;
					new_point->next = temp;
					return 1;
				}
			}
			
			if(point_Y(new_point->pt) < point_Y(temp->pt)){
				if(pre == NULL)
					sp->head = new_point;
				else
					pre->next = new_point;
				new_point->next = temp;
				return 1;
			}
			
			if(point_Y(new_point->pt) > point_Y(temp->pt)){
				/*consider for special case when two points have same origin_dist*/
				while((temp->next != NULL) && new_point->origin_dist == temp->origin_dist && point_Y(new_point->pt) > point_Y(temp->pt)){
					pre = temp;
					temp = temp->next;
				}

				if(point_Y(new_point->pt) != point_Y(temp->pt)){
					
					if((temp->next == NULL) && point_Y(new_point->pt) > point_Y(temp->pt)){
						temp->next = new_point;
						return 1;
					}
					
					pre->next = new_point;
					new_point->next = temp;
                                	return 1;
				}
			}
			
			if(point_X(new_point->pt) == point_X(temp->pt) && point_Y(new_point->pt) == point_Y(temp->pt)){
				/*new_point already exists in the list*/
				if(pre == NULL)
					sp->head = new_point;
				else
					 pre->next = new_point;
				new_point->next = temp;
				return 1;
			}


		}

	
	
	}

	if(temp->next == NULL){
		temp->next = new_point;
		return 1;
	}

	
	return 0;
}

int
sp_remove_first(struct sorted_points *sp, struct point *ret)
{	
	struct point_node *temp;
	temp = sp->head;
	if(sp->head != NULL){
		ret = point_set(ret,point_X(temp->pt),point_Y(temp->pt));
		sp->head = temp->next;
		free(temp->pt);
		free(temp);
		return 1;
	}
	return 0;
}

int
sp_remove_last(struct sorted_points *sp, struct point *ret)
{
	struct point_node *temp;
	struct point_node *pre;
	temp = sp->head;
	pre = NULL;
	if(sp->head == NULL)
		return 0;
	if(sp->head->next == NULL){
		ret = point_set(ret,point_X(temp->pt),point_Y(temp->pt));
		free(temp->pt);
		free(temp);
		sp->head = NULL;
		return 1;
	}
	while(temp->next != NULL){
		pre = temp;
		temp = temp->next;
	}

	ret = point_set(ret,point_X(temp->pt),point_Y(temp->pt));
	
	free(temp->pt);
	free(temp);

	pre->next = NULL;
	return 1;
}

int
sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
	int count=0;
	struct point_node *temp;
	struct point_node *pre;
	if(sp->head == NULL)
		return 0;
	temp = sp->head;
	pre = NULL;

	if(index == count){/*index 0*/
		ret = point_set(ret,point_X(sp->head->pt),point_Y(sp->head->pt));
		sp->head = sp->head->next;
		free(temp->pt);
		free(temp);
		return 1;
	}
	
	while(temp != NULL){
		
		if(count == index){
			ret = point_set(ret,point_X(temp->pt),point_Y(temp->pt));
			pre->next = temp->next;
			free(temp->pt);
			free(temp);
			return 1;
		}
		count++;
		pre = temp;
		temp = temp->next;
	}
	return 0;
}

int
sp_delete_duplicates(struct sorted_points *sp)
{
	int count=0;
	struct point_node *temp;
	struct point_node *pre;
	if(sp->head == NULL)
		return 0;
	if(sp->head->next == NULL)
		return 0;
	temp = sp->head->next;
	pre = sp->head;
	while(temp != NULL && pre != NULL){
		
	
		if(point_X(pre->pt) == point_X(temp->pt) && point_Y(pre->pt) == point_Y(temp->pt)){
			count++;
			pre->next = temp->next;
			free(temp->pt);
			free(temp);
			temp = pre->next;
		}else{
			pre = temp;
			temp = temp->next;
		}
			
		
	}

	return count;
}
