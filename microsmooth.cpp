#include "microsmooth.h"
#include <Arduino.h>
//#include <Serial.h>

float sq_rt(int n)
{
    int i=40;
    for(int j=0; j<6; j++)
    {
	i = (i + n/i)>>1;
    }
    return i;
}


uint16_t* ms_init(uint8_t algo)
{
    if(algo & SMA) return (uint16_t *)calloc(SMA_LENGTH, sizeof(uint16_t));
    else if(algo & CMA) return NULL; 
    else if(algo & EMA) return NULL;
    else if(algo & SGA) return (uint16_t *)calloc(SGA_LENGTH, sizeof(uint16_t));
    else if(algo & KZA) return (uint16_t *)calloc(KZA_LENGTH, sizeof(uint16_t));
    else if(algo & RDP) return (uint16_t *)calloc(RDP_LENGTH, sizeof(uint16_t));
}

void deinit(uint16_t *ptr)
{
    free(ptr);
}

int sma_filter(int current_value, uint16_t history_SMA[])
{  
    uint16_t sum=0; /*This constrains SMA_LENGTH*/
    uint16_t average=0;
    uint8_t i;

    for(i=1;i<SMA_LENGTH;i++)
    {
	history_SMA[i-1]=history_SMA[i];
    }
    history_SMA[SMA_LENGTH-1]=current_value;
    
    for(i=0;i<SMA_LENGTH;i++)
    {
	sum+=history_SMA[i];
    }
    average=sum/SMA_LENGTH;

    return average;
}

int cma_filter(int current_value, void * ptr)  //Seems useless
{ 
    static uint16_t cumulative_average=0; /*Limiting factor for k*/
    static uint16_t k=0; /*Limiting factor for runtime*/
    
    cumulative_average=(current_value+k*(uint32_t)cumulative_average)/(k+1);
    k++;

    return cumulative_average;
}

int ema_filter(int current_value, void * ptr)
{ 
    static uint16_t exponential_average=current_value;
    
    exponential_average=(EMA_ALPHA*(uint32_t)current_value + (100 - EMA_ALPHA)*(uint32_t)exponential_average)/100;
    return exponential_average;
}


const int16_t coefficients[]={1512,-3780,-840,5040,10080,12012,10080,5040,-840,-3780,1512};
const uint16_t normalization_value=36036;


int sga_filter(int current_value, uint16_t history_SGA[])
{ 
    uint64_t sum=0;
    uint8_t SGA_MID = SGA_LENGTH/2;
    uint8_t i;
    
    for(i=1;i<SGA_LENGTH;i++)
    {
	history_SGA[i-1]=history_SGA[i];
    }
    history_SGA[SGA_LENGTH-1]=current_value;
    
    for(i=-SGA_MID;i<=(SGA_MID);i++)
    {  
	sum+=history_SGA[i+SGA_MID]*coefficients[i+SGA_MID];
    }
    
    history_SGA[SGA_MID]=sum/normalization_value;
    return history_SGA[SGA_MID];
}
    
//int history_RDP[RDP_LENGTH] = {0,};
/*
  
void rdp_recur(int start_index, int end_index, uint16_t history_RDP[])
{
  int a = history_RDP[end_index]-history_RDP[start_index];
  int b = -(end_index-start_index);
  int c = -start_index*a - history_RDP[start_index]*b;
  
  float max_distance = 0;
  int max_index=0;
  float distance = 0;
  
  for( int i=start_index+1;i<end_index;i++)
  {
    distance=(i*a-history_RDP[i]*b+c)/sqrt(a^2+b^2);
    if (distance>max_distance)
    { 
      max_distance=distance;
      max_index=i;
    }
  }
  
  if(max_distance>epsilon)
  { 
      rdp_recur(start_index,max_index, history_RDP);
      rdp_recur(max_index,end_index, history_RDP);
   } 
   else 
    
   {
    for( int i=start_index+1;i<end_index;i++) 
    {
      history_RDP[i]=(-c-a*i)/b;
    }
   }
}
*/
   
int rdp_filter(int current_value, uint16_t history_RDP[])
{ 
    int16_t a; 
    int8_t b;
    int16_t c;
    int8_t top=-1;
    int8_t istack[2][RDP_LENGTH], i, j, start_index, end_index, max_index;
    double max_distance, distance, denom; 

    for(i=1;i<RDP_LENGTH;i++)
    { 
	history_RDP[i-1]=history_RDP[i];  
    }
    history_RDP[RDP_LENGTH-1]=current_value;

//rdp_recur(0,RDP_LENGTH-1); /*Recursive one*/
//rdp_iter()
  
    top++;
    istack[0][top]=0;
    istack[1][top]=RDP_LENGTH-1;

//    Serial.println("Going in");
    while(top>-1)
    {
	start_index = istack[0][top];
	end_index = istack[1][top];
	top--;

	a = history_RDP[end_index]-history_RDP[start_index];
	b = -(end_index-start_index);
	c = -start_index*a - history_RDP[start_index]*b;
  
	//Serial.print("a:");Serial.print(a);
	//Serial.print("b:");Serial.print(b);
	//Serial.print("c:");Serial.println(c);
	max_distance = epsilon;
	max_index = start_index;
	distance = 0;
	denom = sq_rt(a*a+b*b);
	
	for(i=start_index+1;i<end_index;i++)
	{
	    distance=abs((i*a-history_RDP[i]*b+c)/denom);
//	    Serial.print("Dist:");
//	    Serial.println(distance);
	    if(distance>max_distance)
	    { 
		max_distance=distance;
		max_index=i;
	    }
	}

//	Serial.print("MaxIndex:");
//	Serial.println(max_index);  
	if(max_index!=start_index)
	{
//	    Serial.println("In push");
	    top++;
	    istack[0][top]=start_index;
	    istack[1][top]=max_index;
	    top++;
	    istack[0][top]=max_index;
	    istack[1][top]=end_index;
	} 
	else 
	{
//	    	Serial.println("in here");
	    for(i=start_index+1;i<end_index;i++) 
	    {
		history_RDP[i]=abs(int((-c-a*i)/b)); 
                //Will this be negative?
//		Serial.println(history_RDP[i]);
	    }
	}
//	Serial.println(top);
    }
    return history_RDP[0];
}


//int history_KZ[KZ_history_LENGTH] = {0,};
//int KZ_MID=(KZ_history_LENGTH)/2;
const long coefficients_k[][17]={{0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0},
                                 {0,0,0,0,1,2,3,4,5,4,3,2,1,0,0,0,0},
                                 {0,0,1,3,6,10,15,18,19,18,15,10,6,3,1,0,0},
                                 {1,4,10,20,35,52,68,80,85,80,68,52,35,20,10,4,1}};
//const long coefficients_k3[]={1,3,6,10,15,18,19,18,15,10,6,3,1};
//const long coefficients_k4[]={1,4,10,20,35,52,68,80,85,80,68,52,35,20,10,4,1};

int kza_filter(int current_value, uint16_t history_KZA[])
{ 
  uint32_t divisor=1;
  uint32_t updated_value=0; 
  int8_t i,k;

  for(i=1;i<KZA_HISTORY_LENGTH;i++)
  {
         history_KZA[i-1]=history_KZA[i];
  }
  history_KZA[KZA_HISTORY_LENGTH-1]=current_value;
    
  for(k=1;k<=3;k++)
  {
    for(i=-k*(KZA_LENGTH-1)/2;i<=k*(KZA_LENGTH-1)/2;i++)
    {
      updated_value+=history_KZA[i+KZA_MID]*coefficients_k[k-1][i+KZA_MID];
    } 
    divisor*=KZA_LENGTH;
    updated_value/=divisor;
    history_KZA[KZA_MID]=updated_value;
   
    updated_value=0;
  } 

  return history_KZA[KZA_MID];
}
