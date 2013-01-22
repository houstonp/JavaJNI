#include "za_co_technodev_javajni_Store.h"
#include "Store.h"
#include "StoreWatcher.h"
#include <stdint.h>
#include <string.h>

static Store gStore = { {}, 0 };
static Store mStore;
static StoreWatcher mStoreWatcher;

/*
 * mInteger which is a C int can be casted directly to a Java jint primitive and vice versa
 */

JNIEXPORT jint JNICALL Java_za_co_technodev_javajni_Store_getInteger
  (JNIEnv* pEnv, jobject pThis, jstring pKey) {
	StoreEntry* lEntry = findEntry(pEnv, &gStore, pKey, NULL);
	if (isEntryValid(pEnv, lEntry, StoreType_Integer)) {
		return lEntry->mValue.mInteger;
	} else {
		return 0.0f;
	}
}

JNIEXPORT void JNICALL Java_za_co_technodev_javajni_Store_setInteger
  (JNIEnv* pEnv, jobject pThis, jstring pKey, jint pInteger) {
	StoreEntry* lEntry = allocateEntry(pEnv, &gStore, pKey);
	if (lEntry != NULL) {
		lEntry->mType = StoreType_Integer;
		lEntry->mValue.mInteger = pInteger;
	}
}

/*
 * Java strings are not real primitives. Types jstring and char* cannot be used interchangeably
 * To create a Java string object from a C string, use NewStringUTF()
 * To convert a Java string into a C string, use GetStringUTFChars() and SetStringUTFChars()
 *
 */

JNIEXPORT jstring JNICALL Java_za_co_technodev_javajni_Store_getString
  (JNIEnv* pEnv, jobject pThis, jstring pKey) {
	StoreEntry* lEntry = findEntry(pEnv, &gStore, pKey, NULL);
	if (isEntryValid(pEnv, lEntry, StoreType_String)) {
		return (*pEnv)->NewStringUTF(pEnv, lEntry->mValue.mString);
	}
	else {
		return NULL;
	}
}

JNIEXPORT void JNICALL Java_za_co_technodev_javajni_Store_setString
  (JNIEnv* pEnv, jobject pThis, jstring pKey, jstring pString) {
	const char* lStringTmp = (*pEnv)->GetStringUTFChars(pEnv, pString, NULL);
	if(lStringTmp == NULL) {
		return;
	}

	StoreEntry* lEntry = allocateEntry(pEnv, &gStore, pKey);
	if (lEntry != NULL) {
		lEntry->mType = StoreType_String;
		jsize lStringLength = (*pEnv)->GetStringUTFLength(pEnv, pString);
		lEntry->mValue.mString = (char*) malloc(sizeof(char) * (lStringLength + 1));
		strcpy(lEntry->mValue.mString, lStringTmp);
	}
	(*pEnv)->ReleaseStringUTFChars(pEnv, pString, lStringTmp);
}

/*
 * Objects passed in parameters or created inside a JNI method are local references. Local references
 * cannot be kept on native code outside method scope. Must use global references to inform Dalvik VM
 * that they cannot be garbage collected. Use NewGlobalRef() and DeleteGlobalRef().
 */

JNIEXPORT jobject JNICALL Java_za_co_technodev_javajni_Store_getColor
  (JNIEnv* pEnv, jobject pThis, jstring pKey) {
	StoreEntry* lEntry = findEntry(pEnv, &gStore, pKey, NULL);
	if (isEntryValid(pEnv, lEntry, StoreType_Color)) {
		return lEntry->mValue.mColor;
	} else {
		return NULL;
	}
}

JNIEXPORT void JNICALL Java_za_co_technodev_javajni_Store_setColor
  (JNIEnv* pEnv, jobject pThis, jstring pKey, jobject pColor) {
	jobject lColor = (*pEnv)->NewGlobalRef(pEnv, pColor);
	if(lColor == NULL) {
		return;
	}

	StoreEntry* lEntry = allocateEntry(pEnv, &gStore, pKey);
	if (lEntry != NULL) {
		lEntry->mType = StoreType_Color;
		lEntry->mValue.mColor = lColor;
	} else {
		(*pEnv)->DeleteGlobalRef(pEnv, lColor);
	}
}

/*
 * An int* array is not equivalent to a jintArray. int* is a reference to an object
 * To return a jintArray here, instantiate a new Java integer array with JNI API
 * method NewIntArray(). Then, use SetIntArrayRegion() to copy the native int buffer
 * content into the jintArray
 */

JNIEXPORT jintArray JNICALL Java_za_co_technodev_javajni_Store_getIntegerArray
  (JNIEnv* pEnv, jobject pThis, jstring pKey) {
	StoreEntry* lEntry = findEntry(pEnv, &gStore, pKey, NULL);
	if (isEntryValid(pEnv, lEntry, StoreType_IntegerArray)) {
		jintArray lJavaArray = (*pEnv)->NewIntArray(pEnv, lEntry->mLength);
		if (lJavaArray == NULL) {
			return;
		}
		(*pEnv)->SetIntArrayRegion(pEnv, lJavaArray, 0 , lEntry->mLength, lEntry->mValue.mIntegerArray);
		return lJavaArray;
	} else {
		return NULL;
	}
}

/*
 * To save a Java array in native code, the inverse operation GetIntArrayRegion() exists.
 * The only way to allocate a suitable target memory buffer is to measure array size with
 * GetArrayLength(). GetIntArrayRegion() also performs bound checking and can raise an exception.
 */

JNIEXPORT void JNICALL Java_za_co_technodev_javajni_Store_setIntegerArray
  (JNIEnv* pEnv, jobject pThis, jstring pKey, jintArray pIntegerArray) {
	jsize lLength = (*pEnv)->GetArrayLength(pEnv, pIntegerArray);
	int32_t* lArray = (int32_t*) malloc(lLength * sizeof(int32_t));
	(*pEnv)->GetIntArrayRegion(pEnv, pIntegerArray, 0, lLength, lArray);
	if ((*pEnv)->ExceptionCheck(pEnv)) {
		free(lArray);
		return;
	}

	StoreEntry* lEntry = allocateEntry(pEnv, &gStore, pKey);
	if(lEntry != NULL) {
		lEntry->mType = StoreType_IntegerArray;
		lEntry->mLength = lLength;
		lEntry->mValue.mIntegerArray = lArray;
	} else {
		free(lArray);
		return;
	}
}

/*
 * Object arrays are represented with type jobjectArray. On the opposite of primitive arrays
 * it is not possible to work on all elements at the same time. Instead, objects are set one by
 * one with SetObjectArrayElement().
 */

JNIEXPORT jobjectArray JNICALL Java_za_co_technodev_javajni_Store_getColorArray
  (JNIEnv* pEnv, jobject pThis, jstring pKey) {
	StoreEntry* lEntry = findEntry(pEnv, &gStore, pKey, NULL);
	if (isEntryValid(pEnv, lEntry, StoreType_ColorArray)) {
		jclass lColorClass = (*pEnv)->FindClass(pEnv, "za/co/technodev/javajni/Color");
		if (lColorClass == NULL) {
			return NULL;
		}
		jobjectArray lJavaArray = (*pEnv)->NewObjectArray(pEnv, lEntry->mLength, lColorClass, NULL);
		(*pEnv)->DeleteLocalRef(pEnv, lColorClass);
		if (lJavaArray == NULL) {
			return NULL;
		}

		int32_t i;
		for (i = 0; i < lEntry->mLength; i++) {
			(*pEnv)->SetObjectArrayElement(pEnv, lJavaArray, i, lEntry->mValue.mColorArray[i]);
			if((*pEnv)->ExceptionCheck(pEnv)) {
				return NULL;
			}
		}
		return lJavaArray;
	} else {
		return NULL;
	}
}

/*
 * Array elements are also retrieved one by one with GetObjectArrayElement(). Returned references
 * are local and should be made global to store them safely in a memeory buffer. If a problem happens
 * global references must be carefully destroyed to allow garbage collection.
 */

JNIEXPORT void JNICALL Java_za_co_technodev_javajni_Store_setColorArray
  (JNIEnv* pEnv, jobject pThis, jstring pKey, jobjectArray pColorArray) {
	jsize lLength = (*pEnv)->GetArrayLength(pEnv, pColorArray);
	jobject* lArray = (jobject*) malloc(lLength * sizeof(jobject));
	int32_t i,j;
	for (i = 0; i < lLength; ++i) {
		jobject lLocalColor = (*pEnv)->GetObjectArrayElement(pEnv, pColorArray, i);
		if(lLocalColor == NULL) {
			for (j = 0; j < i; ++j) {
				(*pEnv)->DeleteGlobalRef(pEnv, lArray[j]);
			}
			free(lArray);
			return;
		}
		lArray[i] = (*pEnv)->NewGlobalRef(pEnv, lLocalColor);
		if(lArray[i] == NULL) {
			for (j = 0; j < i; ++j) {
				(*pEnv)->DeleteGlobalRef(pEnv, lArray[j]);
			}
			free(lArray);
			return;
		}
		(*pEnv)->DeleteLocalRef(pEnv, lLocalColor);
	}

	StoreEntry* lEntry = allocateEntry(pEnv, &gStore, pKey);
	if (lEntry != NULL) {
		lEntry->mType = StoreType_ColorArray;
		lEntry->mLength = lLength;
		lEntry->mValue.mColorArray = lArray;
	} else {
		for (j = 0; j < i; ++j) {
			(*pEnv)->DeleteGlobalRef(pEnv, lArray[j]);
		}
		free(lArray);
		return;
	}
}


JNIEXPORT void JNICALL Java_za_co_technodev_javajni_Store_initializeStore
  (JNIEnv* pEnv, jobject pThis) {
	mStore.mLength = 0;
	startWatcher(pEnv, &mStoreWatcher, &mStore, pThis);
}

JNIEXPORT void JNICALL Java_za_co_technodev_javajni_Store_finalizeStore
  (JNIEnv* pEnv, jobject pThis) {
	stopWatcher(pEnv, &mStoreWatcher);

	StoreEntry* lEntry = mStore.mEntries;
	StoreEntry* lEntryEnd = lEntry + mStore.mLength;
	while (lEntry < lEntryEnd) {
		free(lEntry->mKey);
		releaseEntryValue(pEnv, lEntry);

		++lEntry;
	}
	mStore.mLength = 0;
}
