#include "stdafx.h"
//#include <stdio.h>
//#include <stdlib.h>
#include<iostream>
#include <fstream>
#include <string.h>
#include "jvmpi.h"
#include "jvmdi.h"


using namespace std;

static JVMDI_Interface_1 *jvmdi = NULL;
static string XMLFile = "model.xml";
//static JVMPI_Interface *jvmpi = NULL;

bool CheckModel(string xmlFile)
{
    return true;
}

void ThreadHookEvents(JNIEnv *env , JVMDI_Event *event){
    JVMDI_thread_info info;
    jvmdiError err;
    static int WM_Init_Is_Ok=-1 ;

    printf("tips from bbsunchen: method entrying...");

    if(CheckModel(XMLFile))
    {
        //do if model is good
    }else if(CheckModel(XMLFile))
    {
        //do if model is bad
    }
    //It is safe to call any JNI functions
    //  only after this event is notified(JVMDI_EVENT_VM_INIT)

    /*if ((*event).kind==JVMDI_EVENT_VM_INIT){
        WM_Init_Is_Ok=1;
        fprintf(stderr,"\nJVMDI client is OK ....\n\n");
    }
    if (WM_Init_Is_Ok<0)
        return;
    // Now, JVMDI client is completed install
    //JVMDI_EVENT_THREAD_START
    if ((*event).kind==JVMDI_EVENT_THREAD_START){
        //Get thread info
        err=jvmdi->GetThreadInfo((*event).u.thread_change.thread,&info);
        if (err==0)
            // print record
            fprintf(stderr,"THREAD START (OBJ=%x ,name=%s)\n",info.context_class_loader);
    };
    //JVMDI_EVENT_THREAD_END
    if ((*event).kind==JVMDI_EVENT_THREAD_END){
        //Get thread info
        err=jvmdi->GetThreadInfo((*event).u.thread_change.thread,&info);
        if (err==0)
            // print record
            fprintf(stderr,"THREAD END (OBJ=%x ,name=%s)\n",info.context_class_loader,info.name);
    };
    //JVMDI_EVENT_VM_DEATH
    if ((*event).kind==JVMDI_EVENT_VM_DEATH){
        fprintf(stderr,"\nAplication is finishing\n ....");
    }*/

}

JNIEXPORT jint JNICALL JVM_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
    jvmdiError error;

    //jvm->GetEnv((void **)&jvmdi, JVMDI_VERSION_1);
    error = (*jvm).GetEnv((void **)&jvmdi, JVMDI_VERSION_1);

    if(error < 0)
    {
        return JNI_ERR;
    }

    error = (*jvmdi).SetEventNotificationMode(JVMDI_ENABLE,JVMDI_EVENT_METHOD_ENTRY, (jthread)NULL);
    if(error  == JVMDI_ERROR_INVALID_THREAD)
    {
        printf("warning by bbsunchen: invalid thread\n");
    }else if(error == JVMDI_ERROR_INVALID_EVENT_TYPE)
    {
        printf("warning by bbsunchen: invalid event type\n");
    }

    error = jvmdi->SetEventHook(&ThreadHookEvents);
    if(error < 0)
    {
        return JNI_ERR;
    }
    //jvm->GetEnv((void **)&jvmpi, JVMPI_VERSION_1);

    //jvmpi->NotifyEvent = notifyEvent;
    //jvmpi->EnableEvent(JVMPI_EVENT_CLASS_LOAD, NULL);
    //jvmdi->EnableEvent();

    return JNI_OK;
}

typedef struct {

    //jvmtiEnv      *jvmti;
    jvmdiEnv *jvmti;
    jboolean       vm_is_started;

    jrawMonitorID  lock;
} GlobalAgentData;

//static GlobalAgentData *gdata;


static jlong combined_size;
static int num_class_refs;
static int num_field_refs;
static int num_array_refs;
static int num_classloader_refs;
static int num_signer_refs;
static int num_protection_domain_refs;
static int num_interface_refs;
static int num_static_field_refs;
static int num_constant_pool_refs;





static void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str)
{
    if ( errnum != JVMTI_ERROR_NONE ) {
        char       *errnum_str;

        errnum_str = NULL;
        //(void)(*jvmti).GetErrorName(jvmti, errnum, &errnum_str);  //the error that function does not take 3 arguments
        (void)(*jvmti).GetErrorName(errnum,&errnum_str);

        printf("ERROR: JVMTI: %d(%s): %s\n", errnum, (errnum_str==NULL?"Unknown":errnum_str), (str==NULL?"":str));
    }
}


// Enter a critical section by doing a JVMTI Raw Monitor Enter
static void enter_critical_section(jvmtiEnv *jvmti)
{
    jvmtiError error;

    //error = (*jvmti).RawMonitorEnter(jvmti, gdata->lock);  //the error function does not take 2 arguments
    error = (*jvmti).RawMonitorEnter(gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot enter with raw monitor");
}

// Exit a critical section by doing a JVMTI Raw Monitor Exit
static void exit_critical_section(jvmtiEnv *jvmti)
{
    jvmtiError error;

    error = (*jvmti).RawMonitorExit( gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot exit with raw monitor");
}

void describe(jvmtiError err) {
      jvmtiError err0;
      char *descr;
      err0 = (*jvmti).GetErrorName( err, &descr);
      if (err0 == JVMTI_ERROR_NONE) {
          printf(descr);
      } else {
          printf("error [%d]", err);
      }
 }


// Exception callback
 static void JNICALL callbackException(jvmtiEnv *jvmti_env, JNIEnv* env, jthread thr, jmethodID method, jlocation location, jobject exception, jmethodID catch_method, jlocation catch_location) {
enter_critical_section(jvmti); {

      jvmtiError err, err1, err2, error;
      jvmtiThreadInfo info, info1;
      jvmtiThreadGroupInfo groupInfo;
      jint num_monitors;
      jobject *arr_monitors;

      jvmtiFrameInfo frames[10000];
      jint count;
      jint flag = 0;
      jint thr_st_ptr;//线程状态指针
      jint thr_count;//线程数量
      jthread *thr_ptr;

      jvmtiError err3;
      char *name;
      char *sig;
      char *gsig;

      err3  = (*jvmti).GetMethodName( method, &name, &sig, &gsig);
      if (err3  != JVMTI_ERROR_NONE) {
          printf("GetMethodName:%d\n", err);
          return;
      }

      printf("Got Exception from Method:%s%s\n", name, sig);


      err = (*jvmti).GetThreadInfo(thr, &info);
      if (err != JVMTI_ERROR_NONE) {
          printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
	  describe(err);
                 jvmtiPhase phase;
                 jvmtiError phaseStat;
		 phaseStat = (*jvmti).GetPhase(&phase);
                 printf("    current phase is %d\n", phase);
          printf("\n");

      }


      if (err == JVMTI_ERROR_NONE) {
  	err1 = (*jvmti).GetThreadGroupInfo(info.thread_group, &groupInfo);
                 if (err1 != JVMTI_ERROR_NONE)
                 {
          		printf("(GetThreadGroupInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
	  		describe(err);
          		printf("\n");
                 }
             }

      if ((err == JVMTI_ERROR_NONE ) && (err1 == JVMTI_ERROR_NONE ) )
             {
                 printf("Current Thread is : %s and it belongs to Thread Group :  %s\n", info.name, groupInfo.name);
             }


     err = (*jvmti).GetOwnedMonitorInfo(thr, &num_monitors, &arr_monitors);
      if (err != JVMTI_ERROR_NONE) {
          printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
	  describe(err);
       	  printf("\n");

      }

     printf("Number of Monitors Owned by this thread : %d\n", num_monitors);

     // Get Thread Status
     err = (*jvmti).GetThreadState(thr, &thr_st_ptr);
     if (err != JVMTI_ERROR_NONE) {
          printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
          describe(err);
          printf("\n");

      }

     if (err == JVMTI_ERROR_NONE) {
	printf("Thread Status\n");
	printf("==============\n");
	if  ( thr_st_ptr & JVMTI_THREAD_STATE_ALIVE) {//意思可能是：如果线程存在并且其状态为alive
	    printf("Thread %s is Alive\n", info.name);
		flag = 1;
	}

	if  ( thr_st_ptr & JVMTI_THREAD_STATE_TERMINATED) {

	   printf("Thread %s has been Terminated\n", info.name);
		flag = 1;
	}

	if ( thr_st_ptr & JVMTI_THREAD_STATE_RUNNABLE ) {

	   printf("Thread %s is Runnable\n", info.name);
		flag = 1;
	}

	if ( thr_st_ptr & JVMTI_THREAD_STATE_WAITING ) {
	   printf("Thread %s waiting\n", info.name);
		flag = 1;
	}

	if ( thr_st_ptr & JVMTI_THREAD_STATE_WAITING_INDEFINITELY ) {
	   printf("Thread %s waiting indefinitely\n", info.name);
		flag = 1;
	}

	if ( thr_st_ptr & JVMTI_THREAD_STATE_WAITING_WITH_TIMEOUT ) {
	   printf("Thread %s waiting with Timeout\n", info.name);
		flag = 1;
	}

	if ( thr_st_ptr & JVMTI_THREAD_STATE_SLEEPING ) {
	   printf("Thread %s Sleeping \n", info.name);
		flag = 1;
	}

	if ( thr_st_ptr & JVMTI_THREAD_STATE_WAITING_FOR_NOTIFICATION ) {
	   printf("Thread %s Waiting for Notification \n", info.name);
		flag = 1;
	}

	if ( thr_st_ptr & JVMTI_THREAD_STATE_IN_OBJECT_WAIT ) {
	   printf("Thread %s is in Object Wait \n", info.name);
		flag = 1;
	}
	if ( thr_st_ptr & JVMTI_THREAD_STATE_PARKED ) {
	   printf("Thread %s is Parked \n", info.name);
		flag = 1;
	}
	if ( thr_st_ptr & JVMTI_THREAD_STATE_BLOCKED_ON_MONITOR_ENTER ) {
	   printf("Thread %s is blocked on monitor enter \n", info.name);
		flag = 1;
	}
	if ( thr_st_ptr & JVMTI_THREAD_STATE_SUSPENDED ) {
	   printf("Thread %s is Suspended \n", info.name);
		flag = 1;
	}
	if ( thr_st_ptr & JVMTI_THREAD_STATE_INTERRUPTED ) {
	   printf("Thread %s is Interrupted \n", info.name);
		flag = 1;
	}
	if ( thr_st_ptr & JVMTI_THREAD_STATE_IN_NATIVE ) {

	   printf("Thread %s is in Native \n", info.name);
		flag = 1;
	}
	 if ( flag != 1 )  {
    	    printf("Illegal value  %d for Thread State\n", thr_st_ptr);
	}

	}

     // Get All Threads
     err = (*jvmti).GetAllThreads(&thr_count, &thr_ptr);
     if (err != JVMTI_ERROR_NONE) {
          printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
          describe(err);
          printf("\n");

      }
      if (err == JVMTI_ERROR_NONE && thr_count >= 1) {
	int i = 0;
	printf("Thread Count: %d\n", thr_count);

	for ( i=0; i < thr_count; i++) {
	    // Make sure the stack variables are garbage free
    	    (void)memset(&info1,0, sizeof(info1));

	    err1 = (*jvmti).GetThreadInfo(thr_ptr[i], &info1);
            if (err1 != JVMTI_ERROR_NONE) {
          	printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err1);
          	describe(err1);
          	printf("\n");
            }

            printf("Running Thread#%d: %s, Priority: %d, context class loader:%s\n", i+1,info1.name, info1.priority,(info1.context_class_loader == NULL ? ": NULL" : "Not Null"));


	    // Every string allocated by JVMTI needs to be freed

	    err2 = (*jvmti).Deallocate((unsigned char *)info1.name);
            if (err2 != JVMTI_ERROR_NONE) {
                printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err2);
                describe(err2);
                printf("\n");
            }



	}
      }


     // Get Stack Trace
     err = (*jvmti).GetStackTrace(thr, 0, 10000, (jvmtiFrameInfo *)&frames, &count);
     if (err != JVMTI_ERROR_NONE) {
          printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
          describe(err);
          printf("\n");

      }
	printf("Number of records filled: %d\n", count);
     if (err == JVMTI_ERROR_NONE && count >=1) {

	char *methodName;
        methodName = "yet_to_call()";
   	char *declaringClassName;
        jclass declaring_class;
	int i=0;

	printf("Exception Stack Trace\n");
	printf("=====================\n");
     	printf("Stack Trace Depth: %d\n", count);

        for ( i=0; i < count; i++) {
		err = (*jvmti).GetMethodName(frames[i].method, &methodName, NULL, NULL);
  	 	if (err == JVMTI_ERROR_NONE) {

               		 err = (*jvmti).GetMethodDeclaringClass(frames[i].method, &declaring_class);
               		 //printf("breakpoint 1");
                         err = (*jvmti).GetClassSignature( declaring_class, &declaringClassName, NULL);
                         if (err == JVMTI_ERROR_NONE) {

	                         printf("method number %d at method %s() in class %s\n", i ,methodName, declaringClassName);

                         }


                }

	}
        printf("\n");

        err = (*jvmti).Deallocate((unsigned char *)methodName);
        err = (*jvmti).Deallocate((unsigned char *)declaringClassName);
    }
 } exit_critical_section(jvmti);

 }
// VM Death callback
     static void JNICALL callbackVMDeath(jvmtiEnv *jvmti_env, JNIEnv* jni_env)
     {
	 enter_critical_section(jvmti); {

		printf("Got VM Death event\n");

	} exit_critical_section(jvmti);

     }


// Get a name for a jthread
static void get_thread_name(jvmtiEnv *jvmti, jthread thread, char *tname, int maxlen)
{
    jvmtiThreadInfo info;//线程信息
    jvmtiError      error;

    // Make sure the stack variables are garbage free
    (void)memset(&info,0, sizeof(info));

    // Assume the name is unknown for now
    (void)strcpy(tname, "Unknown");

    // Get the thread information, which includes the name
    error = (*jvmti).GetThreadInfo(thread, &info);
    check_jvmti_error(jvmti, error, "Cannot get thread info");

    // The thread might not have a name, be careful here.
    if ( info.name != NULL ) {
        int len;

        // Copy the thread name into tname if it will fit
        len = (int)strlen(info.name);
        if ( len < maxlen ) {
            (void)strcpy(tname, info.name);  //strcpy的copy顺序是什么？
        }

        // Every string allocated by JVMTI needs to be freed
	    error = (*jvmti).Deallocate( (unsigned char *)info.name);
            if (error != JVMTI_ERROR_NONE) {
                printf("(get_thread_name) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, error);
                describe(error);
                printf("\n");
            }

    }
}


// VM init callback
     static void JNICALL callbackVMInit(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread)
     {
	enter_critical_section(jvmti); {

	char  tname[MAX_THREAD_NAME_LENGTH];
        static jvmtiEvent events[] = { JVMTI_EVENT_THREAD_START, JVMTI_EVENT_THREAD_END };
        int        i;
	jvmtiFrameInfo frames[5];
	jvmtiError err, err1;
        jvmtiError error;
	jint count;

        // The VM has started.
        printf("Got VM init event\n");
        get_thread_name(jvmti_env , thread, tname, sizeof(tname));
        //printf("callbackVMInit:  %s thread\n", tname);


            //error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread)NULL);
            error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
            check_jvmti_error(jvmti_env, error, "Cannot set event notification");



     } exit_critical_section(jvmti);

     }


// JVMTI callback function.
static jvmtiIterationControl JNICALL reference_object(jvmtiObjectReferenceKind reference_kind,
                jlong class_tag, jlong size, jlong* tag_ptr,
                jlong referrer_tag, jint referrer_index, void *user_data)
{

	combined_size = combined_size + size;

	switch (reference_kind) {

		case JVMTI_REFERENCE_CLASS:
			num_class_refs = num_class_refs + 1;
			break;
                case JVMTI_REFERENCE_FIELD:
			num_field_refs = num_field_refs + 1;
                        break;
                case JVMTI_REFERENCE_ARRAY_ELEMENT:
			num_array_refs = num_array_refs + 1;
                        break;
                case JVMTI_REFERENCE_CLASS_LOADER:
			num_classloader_refs = num_classloader_refs + 1;
                        break;
                case JVMTI_REFERENCE_SIGNERS:
			num_signer_refs = num_signer_refs + 1;
                        break;
                case JVMTI_REFERENCE_PROTECTION_DOMAIN:
			num_protection_domain_refs = num_protection_domain_refs + 1;
                        break;
                case JVMTI_REFERENCE_INTERFACE:
			num_interface_refs = num_interface_refs + 1;
                        break;
                case JVMTI_REFERENCE_STATIC_FIELD:
			num_static_field_refs = num_static_field_refs + 1;
                        break;
                case JVMTI_REFERENCE_CONSTANT_POOL:
			num_constant_pool_refs = num_constant_pool_refs + 1;
                        break;
                default:
                        break;
	}


    return JVMTI_ITERATION_CONTINUE;
}


static void JNICALL callbackVMObjectAlloc(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jobject object, jclass object_klass, jlong size) {


	unsigned char *methodName;
	unsigned char *className;
	unsigned char *declaringClassName;
	jclass declaring_class;
	jvmtiError err;


	if (size > 0) {

		err = (*jvmti).GetClassSignature(object_klass, (char **)&className, NULL);

		if (className != NULL) {
			printf("\ntype %s object allocated with size %d\n", className, (jint)size);
		}

		//print stack trace
		jvmtiFrameInfo frames[10000];
		jint count;
		int i;

		err = (*jvmti).GetStackTrace(NULL, 0, 10000, (jvmtiFrameInfo *)&frames, &count);
		if (err == JVMTI_ERROR_NONE && count >= 1) {
			for (i = 0; i < count; i++) {
				err = (*jvmti).GetMethodName(frames[i].method, (char **)&methodName, NULL, NULL);
				if (err == JVMTI_ERROR_NONE) {

					err = (*jvmti).GetMethodDeclaringClass(frames[i].method, &declaring_class);
					err = (*jvmti).GetClassSignature(declaring_class, (char **)&declaringClassName, NULL);
					if (err == JVMTI_ERROR_NONE) {
						printf("at method %s in class %s\n", methodName, declaringClassName);
					}
				}
			}
		}

		//reset counters
		combined_size  = 0;
		num_class_refs = 0;
		num_field_refs = 0;
		num_array_refs = 0;
		num_classloader_refs = 0;
		num_signer_refs = 0;
		num_protection_domain_refs = 0;
		num_interface_refs = 0;
		num_static_field_refs = 0;
		num_constant_pool_refs = 0;

	        err = (*jvmti).IterateOverObjectsReachableFromObject(object, &reference_object, NULL);
		if ( err != JVMTI_ERROR_NONE ) {
		        printf("Cannot iterate over reachable objects\n");
    		}

		printf("\nThis object has references to objects of combined size %d\n", (jint)combined_size);
		printf("This includes %d classes, %d fields, %d arrays, %d classloaders, %d signers arrays,\n", num_class_refs, num_field_refs, num_array_refs, num_classloader_refs, num_signer_refs);
		printf("%d protection domains, %d interfaces, %d static fields, and %d constant pools.\n\n", num_protection_domain_refs, num_interface_refs, num_static_field_refs, num_constant_pool_refs);

		err = (*jvmti).Deallocate((unsigned char *)className);
		err = (*jvmti).Deallocate((unsigned char *)methodName);
		err = (*jvmti).Deallocate( (unsigned char *)declaringClassName);
	}
}
static void JNICALL callbackMethodEntry(jvmtiEnv *jvmti_env,JNIEnv* jni_env,jthread thread,jmethodID method)
{
	unsigned char *methodName;
	unsigned char *className;
	unsigned char *declaringClassName;
	jclass declaring_class;
	jvmtiError err;
	jvmtiThreadInfo info_ptr;

    jint entry_count_ptr;
    jvmtiLocalVariableEntry* table_ptr;
    jint max_ptr;

    //}
    	jvmtiFrameInfo frames[2];
		jint count;
		int i;

		err = (*jvmti).GetStackTrace(NULL, 0, 1, (jvmtiFrameInfo*)&frames, &count);


		if (err == JVMTI_ERROR_NONE && count >= 1)
		{

			for (i = 0; i < count; i++)
			{
				//err = (*jvmti).GetMethodName(frames.method, (char **)&methodName, NULL, NULL);
				if (err == JVMTI_ERROR_NONE)
				 {

					//err = (*jvmti).GetMethodDeclaringClass(frames[i].method, &declaring_class);
					//err = (*jvmti).GetClassSignature(declaring_class, (char **)&declaringClassName, NULL);
					err = (*jvmti).GetMaxLocals(frames[i].method,&max_ptr);
					if(max_ptr >= 1)
					{
					//printf("max_ptr: %d\n",max_ptr);
					//err = (*jvmti).GetMethodLocation(method,&start_location_ptr,&end_location_ptr);
					//printf("strat_location_ptr:%s+++end_location_ptr:%s",start_location_ptr,end_location_ptr);
					err = jvmti->GetLocalVariableTable(frames[i].method,&entry_count_ptr,&table_ptr);
			        //err = (*jvmti).GetLocalVariableTable(frames[i].method,&entry_count_ptr,&table_ptr);
                      if(err != JVMTI_ERROR_NONE)
                      {
                            //printf("(GetThreadGroupInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
	                        //describe(err);
  		                    //printf("\n");
  		                    if(err == JVMTI_ERROR_ABSENT_INFORMATION)
  		                    {
  		                        printf("No local variable information\n");
  		                    }else
  		                    {
  		                    //  printf("(GetThreadGroupInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
	                        //describe(err);
  		                    //printf("\n");
  		                    }
                      }
                      else if(err == JVMTI_ERROR_NONE)
                      {
                            printf("variable count :%d\n",entry_count_ptr);
                            //printf("the first variable %s",variable_array_ptr[0]->name);
                      }
					}
}


JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved){
      static GlobalAgentData data;
      jvmtiError error;
      jint res;
      jvmtiEventCallbacks callbacks;

     (void)memset((void*)&data, 0, sizeof(data));
     gdata = &data;

      //  We need to first get the jvmtiEnv* or JVMTI environment

      res = (*jvm).GetEnv((void **) &jvmti, JVMTI_VERSION_1_0);

      if (res != JNI_OK || jvmti == NULL)
       {
	// This means that the VM was unable to obtain this version of the
         //   JVMTI interface, this is a fatal error.

        printf("ERROR: Unable to access JVMTI Version 1 (0x%x),"
                " is your J2SE a 1.5 or newer version?"
                " JNIEnv's GetEnv() returned %d\n",
               JVMTI_VERSION_1, res);
      }

      // Here we save the jvmtiEnv* for Agent_OnUnload().
    gdata->jvmti = jvmti;


    (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
    capa.can_signal_thread = 1;
    capa.can_get_owned_monitor_info = 1;
    capa.can_generate_method_entry_events = 1;
    capa.can_generate_exception_events = 1;
    capa.can_generate_vm_object_alloc_events = 1;
    capa.can_tag_objects = 1;
    capa.can_access_local_variables = 1;//能够获得本地变量
    capa.can_generate_method_entry_events = 1;//能够使用enrty method开关
    capa.can_signal_thread = 1;//能够interrupt or stop thread
    capa.can_suspend = 1;

    error = (*jvmti).AddCapabilities( &capa);
    if(error == JVMTI_ERROR_NOT_AVAILABLE)
    {
        printf("error!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
    check_jvmti_error(jvmti, error, "Unable to get necessary JVMTI capabilities.");


    (void)memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit = &callbackVMInit; // JVMTI_EVENT_VM_INIT
    callbacks.VMDeath = &callbackVMDeath; // JVMTI_EVENT_VM_DEATH
    callbacks.Exception = &callbackException;// JVMTI_EVENT_EXCEPTION
    callbacks.VMObjectAlloc = &callbackVMObjectAlloc;// JVMTI_EVENT_VM_OBJECT_ALLOC
    callbacks.MethodEntry = &callbackMethodEntry;//JVMTI_EVENT_METHOD_ENTRY



    error = (*jvmti).SetEventCallbacks( &callbacks, (jint)sizeof(callbacks));

    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks");

    error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_VM_INIT, (jthread)NULL);
    error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    //error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
    error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");



    error = (*jvmti).CreateRawMonitor("agent data", &(gdata->lock));
    check_jvmti_error(jvmti, error, "Cannot create raw monitor");


    return JNI_OK;


 }


JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm){

}


