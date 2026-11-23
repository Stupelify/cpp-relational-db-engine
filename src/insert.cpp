#include "catalog.h"
#include "query.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Inserts a record into the specified relation.
 *
 * Returns:
 *  OK on success
 *  an error code otherwise
 */

const Status QU_Insert(const string &relation, const int attrCnt,
                       const attrInfo attrList[]) {

  // print to indicate QU_Insert is being executed
  cout << "Doing QU_Insert " << endl;
  // status variable to track return values of function calls
  Status status;
  // will hold the number of attributes in the relation according to the catalog
  int relAttrCnt;
  // will hold the array of attribute descriptors for the relation from the catalog
  AttrDesc *attrs;

  // get the full attribute information for the relation from the attribute catalog
  status = attrCat->getRelInfo(relation, relAttrCnt, attrs);
  // if the relation was not found or another error occurred, return the error
  if (status != OK)
    return status;

  // if the number of attributes provided does not match the relation's attribute count, reject the insert
  // minirel does not support NULLs so every attribute must be provided
  if (attrCnt != relAttrCnt) {
    free(attrs);
    return ATTRTYPEMISMATCH;
  }

  // compute the total length of the output record by summing all attribute lengths
  int reclen = 0;
  for (int i = 0; i < relAttrCnt; i++) {
    reclen += attrs[i].attrLen;
  }

  // allocate a buffer on the stack to hold the record data
  char recordData[reclen];
  // set up the Record struct pointing to our buffer
  Record rec;
  rec.data = recordData;
  rec.length = reclen;

  // iterate over each attribute in the catalog's ordering for this relation
  for (int i = 0; i < relAttrCnt; i++) {
    // track whether we found a matching attribute in the input list
    bool found = false;

    // search the input attrList for an attribute matching the current catalog attribute by name
    for (int j = 0; j < attrCnt; j++) {
      if (strcmp(attrs[i].attrName, attrList[j].attrName) == 0) {

        // get the string representation of the attribute value from the input list
        const char *strVal = (const char *)attrList[j].attrValue;
        // if the attribute is an integer, convert the string to binary int and copy into record buffer
        if (attrs[i].attrType == INTEGER) {
          int ival = atoi(strVal);
          memcpy(recordData + attrs[i].attrOffset, &ival, attrs[i].attrLen);
        // if the attribute is a float, convert the string to binary float and copy into record buffer
        } else if (attrs[i].attrType == FLOAT) {
          float fval = (float)atof(strVal);
          memcpy(recordData + attrs[i].attrOffset, &fval, attrs[i].attrLen);
        } else {
          // for string attributes, zero out the field first then copy the string value
          memset(recordData + attrs[i].attrOffset, 0, attrs[i].attrLen);
          strncpy(recordData + attrs[i].attrOffset, strVal, attrs[i].attrLen);
        }
        // mark the attribute as found and stop searching the input list
        found = true;
        break;
      }
    }

    // if no matching attribute was found in the input list, reject the insert
    if (!found) {
      free(attrs);
      return ATTRTYPEMISMATCH;
    }
  }

  // open the relation as an InsertFileScan object to insert the new record
  InsertFileScan insertFile(relation, status);
  // if opening the file failed, free attrs and return the error
  if (status != OK) {
    free(attrs);
    return status;
  }

  // rid will hold the record id assigned to the newly inserted record
  RID rid;
  // insert the constructed record into the relation
  status = insertFile.insertRecord(rec, rid);

  // free the dynamically allocated attribute descriptor array
  free(attrs);

  // return OK on success or the error code if insertion failed
  return status;
}