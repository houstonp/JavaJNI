#include "StoreWatcher.h"
#include <unistd.h>

void makeGlobalRef(JNIEnv* pEnv, jobject* pRef);
void deleteGlobalRef(JNIEnv* pEnv, jobject* pRef);
JNIEnv* getJNIEnv(JavaVM* pJavaVM);

void* runWatcher(void* pArgs);
void processEntry(JNIEnv* pEnv, StoreWatcher* pWatcher, StoreEntry* pEntry);
void processEntryInt(JNIEnv* pEnv, StoreWatcher* pWatcher, StoreEntry* pEntry);
void processEntryString(JNIEnv* pEnv, StoreWatcher* pWatcher, StoreEntry* pEntry);
void processEntryColor(JNIEnv* pEnv, StoreWatcher* pWatcher, StoreEntry* pEntry);

/*
 * startWatcher is called from the UI thread to initialize and start the watcher. Thus this is
 * a perfect place to cache JNI descriptors. Actually, this is almost one of the only places
 * because as the UI thread is a Java thread, we have total access to the application class
 * loader. But if we were trying to cache them inside the native thread, the latter would have
 * access only to the system class loader and nothing else.
 */

void startWatcher(JNIEnv* pEnv, StoreWatcher* pWatcher, Store* pStore, jobject pStoreFront) {
	//Erase
	memset(pWatcher, 0, sizeof(StoreWatcher));
	pWatcher->mState = STATE_OK;
	pWatcher->mStore = pStore;

	//Cache the VM
	if ((*pEnv)->GetJavaVM(pEnv, &pWatcher->mJavaVM) != JNI_OK) {
		goto ERROR;
	}

	//Cache classes
	pWatcher->ClassStore = (*pEnv)->FindClass(pEnv, "za/co/technodev/javajni/Store");
	makeGlobalRef(pEnv, &pWatcher->ClassStore);
	if(pWatcher->ClassStore == NULL) {
		goto ERROR;
	}

	pWatcher->ClassColor = (*pEnv)->FindClass(pEnv, "za/co/technodev/javajni/Color");
	makeGlobalRef(pEnv, &pWatcher->ClassColor);
	if(pWatcher->ClassColor == NULL) {
		goto ERROR;
	}

	//Cache Java methods
	pWatcher->MethodOnAlertInt = (*pEnv)->GetMethodID(pEnv, pWatcher->ClassStore, "onAlert", "(I)V");
	if(pWatcher->MethodOnAlertInt == NULL) {
		goto ERROR;
	}

	pWatcher->MethodOnAlertString = (*pEnv)->GetMethodID(pEnv, pWatcher->ClassStore, "onAlert", "(Ljava/lang/String;)V");
	if(pWatcher->MethodOnAlertString == NULL) {
		goto ERROR;
	}

	pWatcher->MethodOnAlertColor = (*pEnv)->GetMethodID(pEnv, pWatcher->ClassStore, "onAlert", "(Lza/co/technodev/javajni/Color;)V");
	if(pWatcher->MethodOnAlertColor == NULL) {
		goto ERROR;
	}

	pWatcher->MethodColorEquals = (*pEnv)->GetMethodID(pEnv, pWatcher->ClassColor, "equals", "(Ljava/lang/Object;)Z");
	if(pWatcher->MethodColorEquals == NULL) {
		goto ERROR;
	}

	jmethodID ConstructorColor = (*pEnv)->GetMethodID(pEnv, pWatcher->ClassColor, "<init>", "(Ljava/lang/String;)V");
	if(ConstructorColor == NULL) {
		goto ERROR;
	}

	//Cache objects
	pWatcher->mStoreFront = (*pEnv)->NewGlobalRef(pEnv, pStoreFront);
	if(pWatcher->mStoreFront == NULL) {
		goto ERROR;
	}

	jstring lColor = (*pEnv)->NewStringUTF(pEnv, "white");
	if (lColor == NULL) {
		goto ERROR;
	}

	pWatcher->mColor = (*pEnv)->NewObject(pEnv, pWatcher->ClassColor, ConstructorColor, lColor);
	makeGlobalRef(pEnv, &pWatcher->mColor);
	if(pWatcher->mColor == NULL) {
		goto ERROR;
	}

	//Init and launch thread
	pthread_attr_t lAttributes;
	int lError = pthread_attr_init(&lAttributes);
	if (lError) {
		goto ERROR;
	}

	lError = pthread_create(&pWatcher->mThread, &lAttributes, runWatcher, pWatcher);
	if (lError) {
		goto ERROR;
	}
	return;

ERROR:
	stopWatcher(pEnv, pWatcher);
	return;
}

/*
 * watcher thread is native which means that no JNI environment is attached. It is not instantiated by Java.
 * the native thread is attached to the VM with AttachCurrentThread() in order to retrieve a JNIEnv.
 * JNIEnv is specific to current thread and cannot be shared with others (opposite with JavaVM). Internally
 * the VM builds a new Thread object and adds it to the main thread group, like any other Java thread.
 */

JNIEnv* getJNIEnv(JavaVM* pJavaVM) {
	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = "NativeThread";
	lJavaVMAttachArgs.group = NULL;

	JNIEnv* lEnv;
	if ((*pJavaVM)->AttachCurrentThread(pJavaVM, &lEnv, &lJavaVMAttachArgs) != JNI_OK) {
		lEnv = NULL;
	}
	return lEnv;
}

/*
 * critical section is delimited with a JNI monitor which has exactly the same properties as the synchronized
 * keyword in java. An attached thread which dies must eventually detach from the VM so that the latter can
 * release resources properly.
 */

void* runWatcher(void* pArgs) {
	StoreWatcher* lWatcher = (StoreWatcher*) pArgs;
	Store* lStore = lWatcher->mStore;
	JavaVM* lJavaVM = lWatcher->mJavaVM;

	JNIEnv* lEnv = getJNIEnv(lJavaVM);
	if (lEnv == NULL) {
		goto ERROR;
	}

	int32_t lRunning = 1;
	while (lRunning) {
		sleep(SLEEP_DURATION);

		StoreEntry* lEntry = lWatcher->mStore->mEntries;
		int32_t lScanning = 1;
		while(lScanning) {
			//Critical section
			(*lEnv)->MonitorEnter(lEnv, lWatcher->mStoreFront);
			lRunning = (lWatcher->mState == STATE_OK);
			StoreEntry* lEntryEnd = lWatcher->mStore->mEntries + lWatcher->mStore->mLength;
			lScanning = (lEntry < lEntryEnd);

			if (lRunning & lScanning) {
				processEntry(lEnv, lWatcher, lEntry);
			}

			//Critical section end
			(*lEnv)->MonitorExit(lEnv, lWatcher->mStoreFront);
			++lEntry;
		}
	}

ERROR:
	(*lJavaVM)->DetachCurrentThread(lJavaVM);
	pthread_exit(NULL);
}

/*
 * To invoke a Java method on a Java object, simply use CallVoidMethod() on a JNI environment.
 * This means that the called Java method returns void. If Java method was returning an int,
 * we would call CallIntMethod(). Like with the reflection API, invoking a Java method method
 * requires:
 *
 * An object instance (except for static methods, in which case we would provide a class instance
 * and use CallStaticVoidMethod().
 *
 * A method descriptor
 *
 * Parameters (if applicable, here an integer value).
 */

void processEntry(JNIEnv* pEnv, StoreWatcher* pWatcher, StoreEntry* pEntry) {
	if ((pEntry->mType == StoreType_Integer) && (strcmp(pEntry->mKey, "watcherCounter") == 0)) {
		++pEntry->mValue.mInteger;
	}
}

void makeGlobalRef(JNIEnv* pEnv, jobject* pRef) {
	if (*pRef != NULL) {
		jobject lGlobalRef = (*pEnv)->NewGlobalRef(pEnv, *pRef);
		(*pEnv)->DeleteLocalRef(pEnv, *pRef);
		*pRef = lGlobalRef;
	}
}

void deleteGlobalRef(JNIEnv* pEnv, jobject* pRef) {
	if (*pRef != NULL) {
		(*pEnv)->DeleteGlobalRef(pEnv, *pRef);
		*pRef = NULL;
	}
}

 void stopWatcher(JNIEnv* pEnv, StoreWatcher* pWatcher) {
	 if (pWatcher->mState == STATE_OK) {
		 (*pEnv)->MonitorEnter(pEnv, pWatcher->mStoreFront);
		 pWatcher->mState = STATE_KO;
		 (*pEnv)->MonitorExit(pEnv, pWatcher->mStoreFront);
		 pthread_join(pWatcher->mThread, NULL);

		 deleteGlobalRef(pEnv, &pWatcher->mStoreFront);
	 }
 }
