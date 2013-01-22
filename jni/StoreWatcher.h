#ifndef _STOREWATCHER_H_
#define _STOREWATCHER_H_

#include "Store.h"
#include <jni.h>
#include <stdint.h>
#include <pthread.h>

#define SLEEP_DURATION 5
#define STATE_OK 0
#define STATE_KO 1

typedef struct {
	//Native variables
	Store* mStore;
	//Cached JNI references
	JavaVM* mJavaVM;
	jobject mStoreFront;
	jobject mColor;
	//Classes
	jclass ClassStore;
	jclass ClassColor;
	//Methods
	jmethodID MethodOnAlertInt;
	jmethodID MethodOnAlertString;
	jmethodID MethodOnAlertColor;
	jmethodID MethodColorEquals;
	//Thread variables
	pthread_t mThread;
	int32_t mState;
} StoreWatcher;

void startWatcher(JNIEnv* pEnv, StoreWatcher* pWatcher, Store* pStore, jobject pStoreFront);
void stopWatcher(JNIEnv* pEnv, StoreWatcher* pWatcher);
#endif
