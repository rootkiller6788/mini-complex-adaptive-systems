#include "ace_core.h"
#include "ace_dynamics.h"
#include "ace_market.h"
#include "ace_evolution.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

static unsigned int seed = 12345;
static int passed = 0, failed = 0;

#define T(n) do{printf("  TEST: %-45s",n);}while(0)
#define P() do{printf("PASS\n");passed++;}while(0)
#define F(m) do{printf("FAIL: %s\n",m);failed++;return;}while(0)
#define C(c,m) do{if(!(c)){F(m);}}while(0)

static void t1(void){T("Arthur urn gamma=0.5");double v=ace_arthur_urn_function(0.3,0.5);C(v>0.0&&v<1.0,"range");P();}
static void t2(void){T("Linear urn a=2");double v=ace_linear_urn_function(0.4,2.0,0.0);C(v>0.4,"not increasing");P();}
static void t3(void){T("Sigmoid at inflection");double v=ace_sigmoid_urn_function(0.5,10.0,0.5);C(ace_fequal(v,0.5,0.1),"not centered");P();}
static void t4(void){T("Shannon entropy uniform");double p[]={0.5,0.5};double H=ace_shannon_entropy(p,2);C(H>0.6&&H<0.7,"entropy");P();}
static void t5(void){T("HHI monopoly=1");double s[]={1.0,0.0,0.0};C(ace_fequal(ace_herfindahl_index(s,3),1.0,0.01),"hhi");P();}
static void t6(void){T("HHI uniform=1/n");double s[]={0.25,0.25,0.25,0.25};C(ace_fequal(ace_herfindahl_index(s,4),0.25,0.01),"hhi");P();}
static void t7(void){T("Polya urn alloc+draw");int init[]={1,1,1};ACEPolyaUrn*u=ace_polya_urn_alloc(3,init,1.0);C(u!=NULL,"alloc");C(u->total_balls==3,"total");int d=ace_polya_urn_draw(u,&seed);C(d>=0&&d<3,"draw");C(u->total_balls==4,"add");ace_polya_urn_free(u);P();}
static void t8(void){T("Path history");ACEPathHistory*h=ace_path_history_alloc(100,2);C(h!=NULL,"alloc");C(h->n_technologies==2,"ntech");ace_path_history_free(h);P();}
static void t9(void){T("Lock-in detection");ACEPathHistory*h=ace_path_history_alloc(100,2);C(h!=NULL,"alloc");for(int s=0;s<50;s++){h->market_shares[s*2]=0.9;h->market_shares[s*2+1]=0.1;h->time_points[s]=(double)s;}h->n_steps=50;C(ace_detect_lockin(h,0.7,10)==ACE_FULL_LOCK,"lockin");ace_path_history_free(h);P();}
static void t10(void){T("Gini equal=0");double d[]={1.0,1.0,1.0,1.0};C(ace_compute_gini(d,4)<0.01,"gini");P();}
static void t11(void){T("Gini unequal>0.5");double d[]={0.0,0.0,0.0,10.0};C(ace_compute_gini(d,4)>0.5,"gini");P();}
static void t12(void){T("Softmax sum=1 order");double s[]={1.0,2.0,3.0},p[3];ace_softmax(s,3,0.5,p);C(ace_fequal(p[0]+p[1]+p[2],1.0,0.01),"sum");C(p[2]>p[1]&&p[1]>p[0],"order");P();}
static void t13(void){T("Timeseries append+stats");ACETimeSeries*ts=ace_timeseries_alloc(10);C(ts!=NULL,"alloc");for(int i=0;i<100;i++)ace_timeseries_append(ts,(double)i);C(ts->length==100,"len");ace_timeseries_update_mean(ts);ace_timeseries_update_variance(ts);C(ts->mean>45.0,"mean");C(ts->variance>500.0,"var");ace_timeseries_free(ts);P();}
static void t14(void){T("Hurst random walk");double rw[100];rw[0]=0.0;for(int i=1;i<100;i++)rw[i]=rw[i-1]+ace_random_gaussian(&seed);double H=ace_estimate_hurst(rw,100);C(H>0.3&&H<0.8,"hurst");P();}
static void t15(void){T("NK landscape generation");ACENKFitnessLandscape*nk=ace_nk_landscape_alloc(5,2);C(nk!=NULL,"alloc");ace_nk_generate(nk,&seed);C(nk->n_peaks>0,"peaks");ace_nk_landscape_free(nk);P();}
static void t16(void){T("NK config encode/decode");ACENKFitnessLandscape*nk=ace_nk_landscape_alloc(4,1);C(nk!=NULL,"alloc");bool c[4]={true,false,true,false},d[4];int idx=ace_nk_config_index(c,4);ace_nk_index_to_config(idx,4,d);for(int i=0;i<4;i++)C(c[i]==d[i],"mismatch");ace_nk_landscape_free(nk);P();}
static void t17(void){T("El Farol init");ACEElFarolConfig*cfg=ace_el_farol_config_alloc(100,60,5,10,50);C(cfg!=NULL,"alloc");ace_el_farol_init(cfg,&seed);int s=0;for(int i=0;i<10;i++)s+=cfg->attendance_history[i];C(s>=0,"att");ace_el_farol_config_free(cfg);P();}
static void t18(void){T("Classifier system");ACEClassifierSystem*cs=ace_classifier_system_alloc(100,0.1,0.01);C(cs!=NULL,"alloc");ace_classifier_add(cs,"01##10","BUY",&seed);int m[10];int n=ace_classifier_match(cs,"011010",m,10);C(n>0,"match");ace_classifier_system_free(cs);P();}
static void t19(void){T("Macro state");ACEAgent a[3];for(int i=0;i<3;i++){a[i].id=i;a[i].wealth=100.0*(i+1);a[i].risk_aversion=0.5;a[i].n_strategies=0;a[i].strategy_weights=NULL;a[i].strategy_predictions=NULL;a[i].learning_rate=0.1;a[i].agent_type=ACE_INDUCTIVE_LEARNER;a[i].reasoning=ACE_INDUCTIVE_REASONING;}ACEMacroState ms;ace_compute_macro_state(a,3,&ms);C(ms.n_active_agents==3,"active");C(ms.herfindahl_index>0.33,"hhi");P();}
static void t20(void){T("Crisis detection");ACEMacroState ms={0};ms.market_volatility=0.5;ms.herfindahl_index=0.5;ms.entropy=1.0;ms.n_bankruptcies=50;ms.n_active_agents=100;C(ace_detect_crisis(&ms,0.3,0.25),"crisis");P();}
static void t21(void){T("Technology combine");ACETechnology t1,t2;ace_technology_init_primitive(&t1,0,"T","electron");ace_technology_init_primitive(&t2,1,"R","ohmic");C(t1.n_phenomena==1,"phenom");ACETechnology*t3=ace_technology_combine(&t1,&t2,2,"Amp",&seed);C(t3!=NULL,"combine");C(t3->n_components==2,"comps");ace_technology_free(t3);/* t1,t2 are stack vars */P();}
static void t22(void){T("ACE full model");ACEFullModel*m=ace_full_model_alloc(10,3,100,&seed);C(m!=NULL,"alloc");C(m->n_agents==10,"agents");ace_full_model_period(m);ace_full_model_period(m);double g,v,i;int cr,in;ace_full_model_report(m,&g,&v,&i,&cr,&in);C(m->current_period==2,"period");ace_full_model_free(m);P();}
static void t23(void){T("LCG determinism");unsigned int s1=42,s2=42;C(ace_lcg_step(&s1)==ace_lcg_step(&s2),"lcg");P();}
static void t24(void){T("Stationarity test");double d[30];for(int i=0;i<30;i++)d[i]=ace_random_gaussian(&seed);bool st=ace_test_stationarity(d,30,0.05);(void)st;P();}
int main(void){
    printf("=== test_arthur_cas ===\n\n");
    t1();
    t2();
    t3();
    t4();
    t5();
    t6();
    t7();
    t8();
    t9();
    t10();
    t11();
    t12();
    t13();
    t14();
    t15();
    t16();
    t17();
    t18();
    t19();
    t20();
    t21();
    t22();
    t23();
    t24();
    printf("\n=== %d passed, %d failed ===\n",passed,failed);
    return (failed==0)?0:1;
}
