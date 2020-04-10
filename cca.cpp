#include "basis.h"
#include "cca.h"
#include "cw.h"
#include "preprocessor.h"

#include <string.h>
#include <sys/times.h> //these two h files are for linux
#include <unistd.h>

char * inst;
int seed;

long long ls_no_improv_times;

bool aspiration_active;



static int pick_var(void)
{
	int         i,k,c,v;
	int         best_var;
	lit*		clause_c;

	/**Greedy Mode**/
	/*CCD (configuration changed decreasing) mode, the level with configuation chekcing*/
	if(goodvar_stack_fill_pointer>0)
	{

		//if(goodvar_stack_fill_pointer<balancePar)
		//{
			best_var = goodvar_stack[0];
			for(i=1; i<goodvar_stack_fill_pointer; ++i)
			{
				v=goodvar_stack[i];
				if(score[v]>score[best_var]) best_var = v;
				else if(score[v]==score[best_var])
				{
					//if(unsat_app_count[v]>unsat_app_count[best_var]) best_var = v;
					//else if(unsat_app_count[v]==unsat_app_count[best_var]&&time_stamp[v]<time_stamp[best_var]) best_var = v;
					
					if(time_stamp[v]<time_stamp[best_var]) best_var = v;
				}
			}
			return best_var;
		//}
		/*else
		{
			best_var = goodvar_stack[rand()%goodvar_stack_fill_pointer];
			for(int j=1;j<balancePar;++j)
			{
				v = goodvar_stack[rand()%goodvar_stack_fill_pointer];
				if(score[v]>score[best_var]) best_var = v;
				else if(score[v]==score[best_var])
				{
					//if(unsat_app_count[v]>unsat_app_count[best_var]) best_var = v;
					//else if(unsat_app_count[v]==unsat_app_count[best_var]&&time_stamp[v]<time_stamp[best_var]) best_var = v;
					if(time_stamp[v]<time_stamp[best_var]) best_var = v;
				}
			}
			return best_var;
		}*/
	}
	
	
	/*aspiration*/
	if (aspiration_active)
	{
		best_var = 0;
		for(i=0; i<unsatvar_stack_fill_pointer; ++i)
		{
			if(score[unsatvar_stack[i]]>ave_weight) 
			{
				best_var = unsatvar_stack[i];
				break;
			}
		}

		for(++i; i<unsatvar_stack_fill_pointer; ++i)
		{
			v=unsatvar_stack[i];
			if(score[v]>score[best_var]) best_var = v;
			else if(score[v]==score[best_var] && time_stamp[v]<time_stamp[best_var]) best_var = v;
		}
		
		if(best_var!=0) return best_var;
	}
	/*****end aspiration*******************/

	update_clause_weights();

	/*focused random walk*/

	c = unsat_stack[rand()%unsat_stack_fill_pointer];
	clause_c = clause_lit[c];
	best_var = clause_c[0].var_num;
	for(k=1; k<clause_lit_count[c]; ++k)
	{
		v=clause_c[k].var_num;
		
		//using score
		//if(score[v]>score[best_var]) best_var = v;
		//else if(score[v]==score[best_var]&&time_stamp[v]<time_stamp[best_var]) best_var = v;
		
		//using unweighted make
		if(unsat_app_count[v]>unsat_app_count[best_var]) best_var = v;
		//else if(unsat_app_count[v]==unsat_app_count[best_var] && time_stamp[v]<time_stamp[best_var]) best_var = v;
		else if(unsat_app_count[v]==unsat_app_count[best_var])
		{
			if(score[v]>score[best_var]) best_var = v;
			else if(score[v]==score[best_var]&&time_stamp[v]<time_stamp[best_var]) best_var = v;
		}
	}

	return best_var;
}




//set functions in the algorithm
void settings()
{
	//set_clause_weighting();
	
	//aspiration_active = false; //
}

/*
void local_search(int max_flips)
{
	int flipvar;
     
	for (step = 0; step<max_flips; step++)
	{
		//find a solution
		if(unsat_stack_fill_pointer==0) return;

		flipvar = pick_var();

		flip(flipvar);

		time_stamp[flipvar] = step;
	}
}
*/

void local_search(long long no_improv_times)
{
	int flipvar;
	long long notime = 1 + no_improv_times;
	
	while(--notime)
	{
		step++;
		
		flipvar = pick_var();
		flip(flipvar);
		time_stamp[flipvar] = step;
		
		if(unsat_stack_fill_pointer < this_try_best_unsat_stack_fill_pointer)
		{
			this_try_best_unsat_stack_fill_pointer = unsat_stack_fill_pointer;
			notime = 1 + no_improv_times;
		}
		
		if(unsat_stack_fill_pointer == 0)
		{
			return;
		}
	}
     
	return;
}

void default_settings()
{
	seed = 1;
	ls_no_improv_times = 200000;
	p_scale = 0.3;
	q_scale = 0.7;
	threshold = 50;
	
	aspiration_active = false; //
}

bool parse_arguments(int argc, char ** argv)
{

	bool flag_inst = false;
	default_settings();
	
	for (int i=1; i<argc; i++)
	{
		if(strcmp(argv[i],"-inst")==0)
		{
			i++;
			if(i>=argc) return false;
			inst = argv[i];
			flag_inst = true;
			continue;
		}
		else if(strcmp(argv[i],"-seed")==0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &seed);
			continue;
		}
		
		else if(strcmp(argv[i],"-aspiration")==0)
		{
			i++;
			if(i>=argc) return false;
			int tmp;
			sscanf(argv[i], "%d", &tmp);
			if (tmp==1)
				aspiration_active = true;
			else 	aspiration_active = false;
			continue;
		}
		
		else if(strcmp(argv[i],"-swt_threshold")==0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &threshold);
			continue;
		}
		else if(strcmp(argv[i],"-swt_p")==0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%f", &p_scale);
			continue;
		}
		else if(strcmp(argv[i],"-swt_q")==0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%f", &q_scale);
			continue;
		}
		
		else if(strcmp(argv[i],"-ls_no_improv_steps")==0){
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%lld", &ls_no_improv_times);
			continue;
		}
		else return false;
		
	}
	
	if (flag_inst) return true;
	else return false;

}


int main(int argc, char* argv[])
{
	int     seed,i;
	int		satisfy_flag=0;
	struct 	tms start, stop;
    
    cout<<"c This is CCAnr 2.0 [Version: 2018.01.28] [Author: Shaowei Cai]."<<endl;	
	
	times(&start);

	bool ret = parse_arguments(argc, argv);
	if(!ret) {cout<<"Arguments Error!"<<endl; return -1;}

	//if (build_instance(argv[1])==0)
	if(build_instance(inst) == 0)
	{
		cout<<"Invalid filename: "<< inst <<endl;
		return -1;
	}
	
    //sscanf(argv[2],"%d",&seed);
    
    srand(seed);
    
    if(unitclause_queue_end_pointer>0) preprocess();
    
    build_neighbor_relation();
    
    scale_ave=(threshold+1)*q_scale; //
    
	cout<<"c Instance: Number of variables = "<<num_vars<<endl;
	cout<<"c Instance: Number of clauses = "<<num_clauses<<endl;
	cout<<"c Instance: Ratio = "<<ratio<<endl;
	cout<<"c Instance: Formula length = "<<formula_len<<endl;
	cout<<"c Instance: Avg (Min,Max) clause length = "<<avg_clause_len<<" ("<<min_clause_len<<","<<max_clause_len<<")"<<endl;
	cout<<"c Algorithmic: Random seed = "<<seed<<endl;
	cout<<"c Algorithmic: ls_no_improv_steps = " << ls_no_improv_times << endl;
	cout<<"c Algorithmic: swt_p = " << p_scale << endl;
	cout<<"c Algorithmic: swt_q = " << q_scale << endl;
	cout<<"c Algorithmic: swt_threshold = " << threshold << endl;
	cout<<"c Algorithmic: scale_ave = " << scale_ave << endl;
	if(aspiration_active) cout<<"c Algorithmic: aspiration_active = true" << endl;
	else cout<<"c Algorithmic: aspiration_active = false" << endl;
    
	for (tries = 0; tries <= max_tries; tries++) 
	{
		 settings();
		 
		 init();
	 
		 local_search(ls_no_improv_times);

		 if (unsat_stack_fill_pointer==0) 
		 {
		 	if(verify_sol()==1) {satisfy_flag = 1; break;}
		    else cout<<"c Sorry, something is wrong."<<endl;/////
		 }
	}

	times(&stop);
	double comp_time = double(stop.tms_utime - start.tms_utime +stop.tms_stime - start.tms_stime) / sysconf(_SC_CLK_TCK);

    if(satisfy_flag==1)
    {
    	cout<<"s SATISFIABLE"<<endl;
		print_solution();
    }
    else  cout<<"s UNKNOWN"<<endl;
    
    cout<<"c solveSteps = "<<tries<<" tries + "<<step<<" steps (each try has "<<max_flips<<" steps)."<<endl;
    cout<<"c solveTime = "<<comp_time<<endl;
	 
    free_memory();

    return 0;
}
