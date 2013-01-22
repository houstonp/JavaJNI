#include "Store.h"
#include <string.h>

int32_t isEntryValid(JNIEnv* pEnv, StoreEntry* pEntry, StoreType pType) {
	if (pEntry == NULL) {
		throwNotExistingKeyException(pEnv);
	} else if (pEntry->mType != pType) {
		throwInvalidTypeException(pEnv);
	} else {
		return 1;
	}
	return 0;
}

/* Good practice to check that GetStringUTFChars() does not return a NULL value
 *
 */

StoreEntry* findEntry(JNIEnv* pEnv, Store* pStore, jstring pKey, int32_t* pError) {
	StoreEntry* lEntry = pStore->mEntries;
	StoreEntry* lEntryEnd = lEntry + pStore->mLength;

	const char* lKeyTmp = (*pEnv)->GetStringUTFChars(pEnv, pKey, NULL);

	if (lKeyTmp == NULL) {
		if (pError != NULL) {
			*pError = 1;
		}
		return;
	}

	while ((lEntry < lEntryEnd) && (strcmp(lEntry->mKey, lKeyTmp) != 0)) {
		++lEntry;
	}
	(*pEnv)->ReleaseStringUTFChars(pEnv, pKey, lKeyTmp);

	return (lEntry == lEntryEnd) ? NULL : lEntry;
}

/* Raw JNI objects live for the time of a method and cannot be kept outside its scope
 * Convert key to C string kept in memory outside method scope
 *
 */

StoreEntry* allocateEntry(JNIEnv* pEnv, Store* pStore, jstring pKey) {
	int32_t lError = 0;
	StoreEntry* lEntry = findEntry(pEnv, pStore, pKey, &lError);
	if (lEntry != NULL) {
		releaseEntryValue(pEnv, lEntry);
	} else if (!lError) {
		if (pStore->mLength >= STORE_MAX_CAPACITY) {
			throwStoreFullException(pEnv);
			return NULL;
		}

		//Return last array element
		lEntry = pStore->mEntries + pStore->mLength;

		//Converting jstring to native c string
		const char* lKeyTmp = (*pEnv)->GetStringUTFChars(pEnv, pKey, NULL);

		if (lKeyTmp == NULL) {
			return;
		}

		//Allocating memory
		lEntry->mKey = (char*) malloc(strlen(lKeyTmp));

		//copy c string into mKey location
		strcpy(lEntry->mKey, lKeyTmp);

		(*pEnv)->ReleaseStringUTFChars(pEnv, pKey, lKeyTmp);

		++pStore->mLength;
	}
	return lEntry;
}

/* Free memory allocated for a value
 *
 */

void releaseEntryValue(JNIEnv* pEnv, StoreEntry* pEntry) {
	int32_t i;
	switch (pEntry->mType) {
	case StoreType_String:
		free(pEntry->mValue.mString);
		break;
	case StoreType_Color:
		(*pEnv)->DeleteGlobalRef(pEnv, pEntry->mValue.mColor);
		break;
	case StoreType_IntegerArray:
		free(pEntry->mValue.mIntegerArray);
		break;
	case StoreType_ColorArray:
		for (i = 0; i< pEntry->mLength; i++) {
			(*pEnv)->DeleteGlobalRef(pEnv, pEntry->mValue.mColorArray[i]);
		}
		free(pEntry->mValue.mColorArray);
		break;
	}
}

void throwNotExistingKeyException(JNIEnv* pEnv) {
	jclass lClass = (*pEnv)->FindClass(pEnv, "za/co/technodev/exception/NotExistingKeyException");
	if (lClass != NULL) {
		(*pEnv)->ThrowNew(pEnv, lClass, "Key does not exist");
	}
	(*pEnv)->DeleteLocalRef(pEnv, lClass);
}

void throwInvalidTypeException(JNIEnv* pEnv) {
	jclass lClass = (*pEnv)->FindClass(pEnv, "za/co/technodev/exception/InvalidTypeException");
	if (lClass != NULL) {
		(*pEnv)->ThrowNew(pEnv, lClass, "Invalid type");
	}
	(*pEnv)->DeleteLocalRef(pEnv, lClass);
}

void throwStoreFullException(JNIEnv* pEnv) {
	jclass lClass = (*pEnv)->FindClass(pEnv, "za/co/technodev/exception/StoreFullException");
	if (lClass != NULL) {
		(*pEnv)->ThrowNew(pEnv, lClass, "Store is full");
	}
	(*pEnv)->DeleteLocalRef(pEnv, lClass);
}
