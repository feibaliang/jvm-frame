//
// Created by Administrator on 2015/10/10.
//
#include <string.h>
#include "jvmti.h"

//static JVMPI_Interface *jvmpi = NULL;

typedef struct {

    jvmtiEnv      *jvmti;
    //jvmdiEnv *jvmti;
    jboolean       vm_is_started;

    jrawMonitorID  lock;
} GlobalAgentData;

static GlobalAgentData *gdata;

void thread_status(jvmtiEnv const *jvmti, const struct _jobject *thr, jvmtiError *err, jvmtiThreadInfo *info, jint flag,
                   jint *thr_st_ptr);

void get_all_thread(jvmtiEnv const *jvmti, jvmtiError *err, jvmtiError *err1, jvmtiError *err2, jvmtiThreadInfo *info1,
                    jint *thr_count, jthread **thr_ptr);

void get_statck_trace(jvmtiEnv const *jvmti, const struct _jobject *thr, jvmtiError *err, jvmtiFrameInfo **frames,
                      jint *count);

static void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str)
{
    if (errnum != JVMTI_ERROR_NONE) {
        char       *errnum_str;

        errnum_str = NULL;
        //(void)(*jvmti).GetErrorName(jvmti, errnum, &errnum_str);  //the error that function does not take 3 arguments
        (*jvmti)->GetErrorName(jvmti, errnum, &errnum_str);
        //(void)(*jvmti).GetErrorName(errnum,&errnum_str);

        printf("ERROR: JVMTI: %d(%s): %s\n", errnum, (errnum_str == NULL ? "Unknown" : errnum_str), (str == NULL ? "" : str));
    }
}

// Enter a critical section by doing a JVMTI Raw Monitor Enter
static void enter_critical_section(jvmtiEnv *jvmti)
{
    jvmtiError error;

    error = (*jvmti)->RawMonitorEnter(jvmti, gdata->lock);  //the error function does not take 2 arguments
    //    error = (*jvmti).RawMonitorEnter(gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot enter with raw monitor");
}

// Exit a critical section by doing a JVMTI Raw Monitor Exit
static void exit_critical_section(jvmtiEnv *jvmti)
{
    jvmtiError error;
    error = (*jvmti)->RawMonitorExit(jvmti, gdata->lock);
    //    error = (*jvmti).RawMonitorExit( gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot exit with raw monitor");
}

void describe(jvmtiError err) {
    jvmtiError err0;
    char *descr;
    err0 = (*(gdata->jvmti))->GetErrorName(gdata->jvmti, err, &descr);
    if (err0 == JVMTI_ERROR_NONE) {
        printf("%s\n",descr);
    }
    else {
        printf("error [%d]\n", err);
    }
}

void get_thread_status(jvmtiEnv const *jvmti, const struct _jobject *thr, jvmtiThreadInfo *info, jint flag,
                       jint *thr_st_ptr) {
    jvmtiError err = (*jvmti)->GetThreadState(jvmti, thr, thr_st_ptr);
    if (err != JVMTI_ERROR_NONE) {
        printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
        describe(err);
        printf("\n");

    }

    if (err == JVMTI_ERROR_NONE) {
        printf("Thread Status\n");
        printf("==============\n");
        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_ALIVE) {//“‚Àºø…ƒ‹ «£∫»Áπ˚œﬂ≥Ã¥Ê‘⁄≤¢«“∆‰◊¥Ã¨Œ™alive
            printf("Thread %s is Alive\n", (*info).name);
            flag = 1;
        }

        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_TERMINATED) {

            printf("Thread %s has been Terminated\n", (*info).name);
            flag = 1;
        }

        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_RUNNABLE) {

            printf("Thread %s is Runnable\n", (*info).name);
            flag = 1;
        }

        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_WAITING) {
            printf("Thread %s waiting\n", (*info).name);
            flag = 1;
        }

        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_WAITING_INDEFINITELY) {
            printf("Thread %s waiting indefinitely\n", (*info).name);
            flag = 1;
        }

        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_WAITING_WITH_TIMEOUT) {
            printf("Thread %s waiting with Timeout\n", (*info).name);
            flag = 1;
        }

        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_SLEEPING) {
            printf("Thread %s Sleeping \n", (*info).name);
            flag = 1;
        }

        //            if ( thr_st_ptr & JVMTI_THREAD_STATE_WAITING_FOR_NOTIFICATION ) {
        //                printf("Thread %s Waiting for Notification \n", info.name);
        //                flag = 1;
        //            }

        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_IN_OBJECT_WAIT) {
            printf("Thread %s is in Object Wait \n", (*info).name);
            flag = 1;
        }
        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_PARKED) {
            printf("Thread %s is Parked \n", (*info).name);
            flag = 1;
        }
        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_BLOCKED_ON_MONITOR_ENTER) {
            printf("Thread %s is blocked on monitor enter \n", (*info).name);
            flag = 1;
        }
        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_SUSPENDED) {
            printf("Thread %s is Suspended \n", (*info).name);
            flag = 1;
        }
        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_INTERRUPTED) {
            printf("Thread %s is Interrupted \n", (*info).name);
            flag = 1;
        }
        if ((*thr_st_ptr) & JVMTI_THREAD_STATE_IN_NATIVE) {

            printf("Thread %s is in Native \n", (*info).name);
            flag = 1;
        }
        if (flag != 1) {
            printf("Illegal value  %d for Thread State\n", (*thr_st_ptr));
        }

    }
}

void get_all_thread(jvmtiEnv const *jvmti, jvmtiError *err, jvmtiError *err1, jvmtiError *err2, jvmtiThreadInfo *info1,
                    jint *thr_count, jthread **thr_ptr) {
    (*err) = (*jvmti)->GetAllThreads(jvmti, thr_count, thr_ptr);
    if ((*err) != JVMTI_ERROR_NONE) {
        printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, (*err));
        describe((*err));
        printf("\n");

    }
    if ((*err) == JVMTI_ERROR_NONE && (*thr_count) >= 1) {
        int i = 0;
        printf("Thread Count: %d\n", (*thr_count));

        for (i = 0; i < (*thr_count); i++) {
            // Make sure the stack variables are garbage free
            (void)memset(info1, 0, sizeof((*info1)));

            (*err1) = (*jvmti)->GetThreadInfo(jvmti, (*thr_ptr)[i], info1);
            if ((*err1) != JVMTI_ERROR_NONE) {
                printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, (*err1));
                describe((*err1));
                printf("\n");
            }

            printf("Running Thread#%d: %s, Priority: %d, context class loader:%s\n", i + 1, (*info1).name, (*info1).priority, (
                    (*info1).context_class_loader == NULL ? ": NULL" : "Not Null"));


            // Every string allocated by JVMTI needs to be freed

            (*err2) = (*jvmti)->Deallocate(jvmti, (unsigned char *) (*info1).name);
            if ((*err2) != JVMTI_ERROR_NONE) {
                printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, (*err2));
                describe((*err2));
                printf("\n");
            }



        }
    }
}


void show_statck_trace(jvmtiEnv const *jvmti, const struct _jobject *thr,jint *count) {
    jvmtiFrameInfo frames[10000];
    jvmtiError err = (*jvmti)->GetStackTrace(jvmti, thr, 0, 10000, frames, count);
    if (err != JVMTI_ERROR_NONE) {
        printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
        describe(err);
        printf("\n");

    }
    printf("Number of records filled: %d\n", (*count));
    if (err == JVMTI_ERROR_NONE && (*count) >= 1) {

        char *methodName;
        methodName = "yet_to_call()";
        char *declaringClassName;
        jclass declaring_class;
        int i = 0;

        printf("Exception Stack Trace\n");
        printf("=====================\n");
        printf("Stack Trace Depth: %d\n", (*count));

        for (i = 0; i < (*count); i++) {
            err = (*jvmti)->GetMethodName(jvmti, frames[i].method, &methodName, NULL, NULL);
            if (err == JVMTI_ERROR_NONE) {

                err = (*jvmti)->GetMethodDeclaringClass(jvmti, frames[i].method, &declaring_class);
                err = (*jvmti)->GetClassSignature(jvmti, declaring_class, &declaringClassName, NULL);
                if (err == JVMTI_ERROR_NONE) {

                    printf("method number %d at method %s() in class %s\n", i, methodName, declaringClassName);

                }
                printf("get local variabel table\n");
                jvmtiLocalVariableEntry *localVariableEntry;
                jint local_var_entry_count;
                printf("方法名%s\n",methodName);
                err = (*jvmti)->GetLocalVariableTable(jvmti,frames[i].method,&local_var_entry_count,&localVariableEntry);

                if(err != JVMTI_ERROR_NONE) {
                    printf("error when get local variable table\n");
                    describe(err);
                    return;
                }
                printf("get local value%d\n",local_var_entry_count);
                jobject result;
                jlong jVal;
                jfloat fVal;
                jdouble dVal;
                jint iVal;
                printf("get local value2%d\n",local_var_entry_count);
                for (int j = 0; j < local_var_entry_count; ++j) {
                    printf("%d,%s\n",j,localVariableEntry[j].signature);

                    switch (localVariableEntry[j].signature[0]) {

                        case '[': // Array
                        case 'L': // Object
//                            err = (*jvmti)->GetLocalObject(jvmti, &thr,
//                                                           i, localVariableEntry[j].slot,
//                                                           &result);
                            break;
                        case 'J': // long
                            err = (*jvmti)->GetLocalLong(jvmti, &thr, i, localVariableEntry[j].slot, &jVal);
                            printf("%s=%d\n", localVariableEntry[j].name, jVal);
                            break;
                        case 'F': // float
                            err = (*jvmti)->GetLocalFloat(jvmti, &thr, i, localVariableEntry[j].slot, &fVal);
                            printf("%s=%f\n", localVariableEntry[j].name, fVal);
                            break;
                        case 'D': // double
                            err = (*jvmti)->GetLocalDouble(jvmti, &thr, i, localVariableEntry[j].slot, &dVal);
                            printf("%s=%f\n", localVariableEntry[j].name, dVal);
                            break;
                            // Integer types
                        case 'I': // int
                        case 'S': // short
                        case 'C': // char
                        case 'B': // byte
                        case 'Z': // boolean
                            printf("int");
                            err = (*jvmti)->GetLocalInt(jvmti, &thr, i, localVariableEntry[j].slot, &iVal);
                            printf("%s=%d\n", localVariableEntry[j].name, iVal);
                            break;
                        default:
                            printf("++++++++++++++++居然没有这种类型+++++++++++%s", localVariableEntry[j].signature[0]);
                            break;
                    }
                }
            }

        }
        printf("111111\n");

//        err = (*jvmti)->Deallocate(jvmti, (unsigned char *)methodName);
//        err = (*jvmti)->Deallocate(jvmti, (unsigned char *)declaringClassName);
    }
}

// Exception callback
static void JNICALL callbackException(jvmtiEnv *jvmti, JNIEnv* env, jthread thr, jmethodID method, jlocation location, jobject exception, jmethodID catch_method, jlocation catch_location) {
    enter_critical_section(jvmti); {

        jvmtiError err, err1, err2, error;
        jvmtiThreadInfo info, info1;
        jvmtiThreadGroupInfo groupInfo;
        jint num_monitors;
        jobject *arr_monitors;

        jint count;
        jint flag = 0;
        jint thr_st_ptr;//œﬂ≥Ã◊¥Ã¨÷∏’Î
        jint thr_count;//œﬂ≥Ã ˝¡ø
        jthread *thr_ptr;

        jvmtiError err3;
        char *name;
        char *sig;
        char *gsig;

        err3 = (*jvmti)->GetMethodName(jvmti, method, &name, &sig, &gsig);
        //        err3  = (*jvmti).GetMethodName( method, &name, &sig, &gsig);
        if (err3 != JVMTI_ERROR_NONE) {
            printf("GetMethodName:%d\n", err3);
            return;
        }

        printf("Got Exception from Method:%s%s\n", name, sig);


        err = (*jvmti)->GetThreadInfo(jvmti, thr, &info);
        if (err != JVMTI_ERROR_NONE) {
            printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
            describe(err);
            jvmtiPhase phase;
            jvmtiError phaseStat;
            phaseStat = (*jvmti)->GetPhase(jvmti, &phase);
            printf("    current phase is %d\n", phase);
            printf("\n");

        }

        if (err == JVMTI_ERROR_NONE) {
            err1 = (*jvmti)->GetThreadGroupInfo(jvmti, info.thread_group, &groupInfo);
            if (err1 != JVMTI_ERROR_NONE)
            {
                printf("(GetThreadGroupInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
                describe(err);
                printf("\n");
            }else{
                printf("Current Thread is : %s and it belongs to Thread Group :  %s\n", info.name, groupInfo.name);
            }
        }

        err = (*jvmti)->GetOwnedMonitorInfo(jvmti, thr, &num_monitors, &arr_monitors);
        if (err != JVMTI_ERROR_NONE) {
            printf("(GetThreadInfo) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, err);
            describe(err);
            printf("\n");

        }

        printf("Number of Monitors Owned by this thread : %d\n", num_monitors);

        // Get Thread Status
        get_thread_status(jvmti, thr, &info, flag, &thr_st_ptr);

        // Get All Threads
        get_all_thread(jvmti, &err, &err1, &err2, &info1, &thr_count, &thr_ptr);

        // Get Stack Trace
        show_statck_trace(jvmti, thr, &count);
    } exit_critical_section(jvmti);

}


JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
    static GlobalAgentData data;
    jvmtiError error;
    jint res;
    jvmtiEventCallbacks callbacks;
    jvmtiCapabilities capa;
    jvmtiEnv *jvmti;

    (void)memset((void *)&data, 0, sizeof(data));
    gdata = &data;

    //  We need to first get the jvmtiEnv* or JVMTI environment
    res = (*jvm)->GetEnv(jvm, (void **)&jvmti, JVMTI_VERSION_1_2);
    //    res = (*jvm).GetEnv((void **) &jvmti, JVMTI_VERSION_1_0);

    if (res != JNI_OK || *jvmti == NULL) {
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
    capa.can_access_local_variables = 1;//ƒ‹πªªÒµ√±æµÿ±‰¡ø
    capa.can_generate_method_entry_events = 1;//ƒ‹πª π”√enrty methodø™πÿ
    capa.can_signal_thread = 1;//ƒ‹πªinterrupt or stop thread
    capa.can_suspend = 1;

    error = (*jvmti)->AddCapabilities(jvmti, &capa);
    if (error == JVMTI_ERROR_NOT_AVAILABLE) {
        printf("error!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
    check_jvmti_error(jvmti, error, "Unable to get necessary JVMTI capabilities.");

    (void)memset(&callbacks, 0, sizeof(callbacks));
    //    callbacks.VMInit = &callbackVMInit; // JVMTI_EVENT_VM_INIT
    //    callbacks.VMDeath = &callbackVMDeath; // JVMTI_EVENT_VM_DEATH
    callbacks.Exception = &callbackException;// JVMTI_EVENT_EXCEPTION
    //    callbacks.VMObjectAlloc = &callbackVMObjectAlloc;// JVMTI_EVENT_VM_OBJECT_ALLOC
    //    callbacks.MethodEntry = &callbackMethodEntry;//JVMTI_EVENT_METHOD_ENTRY



    error = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint) sizeof(callbacks));

    check_jvmti_error(&jvmti, error, "Cannot set jvmti callbacks");

    //    error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_VM_INIT, (jthread)NULL);
    //    error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    //error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, (jthread)NULL);
    //    error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
    error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread)NULL);
    check_jvmti_error(&jvmti, error, "Cannot set event notification");


    error = (*jvmti)->CreateRawMonitor(jvmti, "agent data", &(gdata->lock));
    check_jvmti_error(&jvmti, error, "Cannot create raw monitor");

    return JNI_OK;
}

