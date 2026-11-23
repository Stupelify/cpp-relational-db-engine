#include "catalog.h"
#include "query.h"
#include "stdio.h"
#include "stdlib.h"
#include <string.h>

// forward declaration of ScanSelect so QU_Select can call it before it is defined
const Status ScanSelect(const string &result, const int projCnt,
                        const AttrDesc projNames[], const AttrDesc *attrDesc,
                        const Operator op, const char *filter,
                        const int reclen);

/*
 * Selects records from the specified relation.
 *
 * Returns:
 *  OK on success
 *  an error code otherwise
 */

const Status QU_Select(const string &result, const int projCnt,
                       const attrInfo projNames[], const attrInfo *attr,
                       const Operator op, const char *attrValue) {
  // print to indicate QU_Select is being executed
  cout << "Doing QU_Select " << endl;
  // status variable to track return values of function calls
  Status status;
  // will hold the total byte length of the output tuple
  int reclen = 0;

  // array of AttrDesc structs for each projected attribute, looked up from the catalog
  AttrDesc projDesc[projCnt];

  // for each projected attribute, look up its descriptor from the catalog
  // and accumulate the total output record length
  for (int i = 0; i < projCnt; i++) {
    status = attrCat->getInfo(projNames[i].relName, projNames[i].attrName,
                              projDesc[i]);
    // if the attribute was not found in the catalog, return the error
    if (status != OK)
      return status;
    reclen += projDesc[i].attrLen;
  }

  // attrDesc will hold the descriptor for the selection condition attribute
  AttrDesc attrDesc;
  // attrDescPtr is NULL if there is no selection condition (unconditional scan)
  AttrDesc *attrDescPtr = NULL;
  // filter starts as the raw string value of the selection condition
  const char *filter = attrValue;
  // declare variables for type conversion of the filter value
  int intVal;
  float floatVal;

  // if a selection condition attribute was provided, look it up in the catalog
  if (attr != NULL) {
    status = attrCat->getInfo(attr->relName, attr->attrName, attrDesc);
    // if the attribute was not found in the catalog, return the error
    if (status != OK)
      return status;
    // set the pointer so ScanSelect knows to do a conditional scan
    attrDescPtr = &attrDesc;

    // if the filter attribute is an integer, convert the string value to binary int
    if (attrDesc.attrType == INTEGER) {
      intVal = atoi(attrValue);
      filter = (char *)&intVal;
    // if the filter attribute is a float, convert the string value to binary float
    } else if (attrDesc.attrType == FLOAT) {
      floatVal = atof(attrValue);
      filter = (char *)&floatVal;
    }
    // for strings, filter already points to the raw string value, no conversion needed
  }

  // call ScanSelect with all the prepared parameters to do the actual scanning and projection
  return ScanSelect(result, projCnt, projDesc, attrDescPtr, op, filter, reclen);
}

const Status ScanSelect(const string &result,

                        const int projCnt, const AttrDesc projNames[],
                        const AttrDesc *attrDesc, const Operator op,
                        const char *filter, const int reclen) {
  // print to indicate ScanSelect is being executed
  cout << "Doing HeapFileScan Selection using ScanSelect()" << endl;
  // status variable to track return values of function calls
  Status status;

  // open the result relation as an InsertFileScan to insert matching output tuples into it
  InsertFileScan resultFile(result, status);
  // if opening the result file failed, return the error
  if (status != OK)
    return status;

  // get the base relation name from the first projected attribute's relation name
  string baseRel = projNames[0].relName;

  // open the base relation as a HeapFileScan to scan through its records
  HeapFileScan scan(baseRel, status);
  // if opening the base relation failed, return the error
  if (status != OK)
    return status;
  // if a selection condition was provided, start a conditional scan
  if (attrDesc != NULL) {
    status = scan.startScan(attrDesc->attrOffset, attrDesc->attrLen,
                            (Datatype)attrDesc->attrType, filter, op);
  } else {
    // no selection condition, start an unconditional scan to return all records
    status = scan.startScan(0, 0, STRING, NULL, op); // Unconditional
  }
  // if starting the scan failed, return the error
  if (status != OK)
    return status;

  // rid will hold the record id of each matching record found during the scan
  RID rid;
  // rec will hold the full record data read from the base relation
  Record rec;
  // buffer to hold the projected output record data
  char outData[reclen];
  // output record struct pointing to our output buffer
  Record outRec;
  outRec.data = outData;
  outRec.length = reclen;
  // scan through all matching records one by one
  while (scan.scanNext(rid) == OK) {
    // read the full record from the base relation
    status = scan.getRecord(rec);
    // if reading the record failed, return the error
    if (status != OK)
      return status;

    // copy only the projected attributes into the output record buffer
    int offset = 0;
    for (int i = 0; i < projCnt; i++) {
      // copy each projected attribute from its offset in the source record to the output buffer
      memcpy(outData + offset, (char *)rec.data + projNames[i].attrOffset,
             projNames[i].attrLen);
      // advance the output offset by the length of the attribute just copied
      offset += projNames[i].attrLen;
    }

    // rid for the newly inserted output record
    RID outRid;
    // insert the projected output record into the result relation
    status = resultFile.insertRecord(outRec, outRid);
    // if insertion failed, return the error
    if (status != OK)
      return status;
  }

  // all matching records have been projected and inserted into the result relation
  return OK;
}