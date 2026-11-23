#include "catalog.h"
#include "query.h"

/*
 * Deletes records from a specified relation.
 *
 * Returns:
 *  OK on success
 *  an error code otherwise
 */

const Status QU_Delete(const string &relation, const string &attrName,
                       const Operator op, const Datatype type,
                       const char *attrValue) {

  // print to indicate QU_Delete is being executed
  cout << "Doing QU_Delete " << endl;
  // status variable to track return values of function calls
  Status status;

  // open the relation as a HeapFileScan object to scan through its records
  HeapFileScan scan(relation, status);
  // if opening the heap file failed, return the error
  if (status != OK)
    return status;

  // if no attribute name or value is given, do an unconditional scan (delete all records)
  if (attrName.empty() || attrValue == NULL) {
    status = scan.startScan(0, 0, STRING, NULL, op);
  } else {
    // look up the attribute descriptor from the catalog to get offset, length, type
    AttrDesc attrDesc;
    status = attrCat->getInfo(relation, attrName, attrDesc);
    // if the attribute was not found in the catalog, return the error
    if (status != OK)
      return status;

    // default filter pointer points to the raw string value
    const char *filter = attrValue;
    // declare variables for type conversion
    int intVal;
    float floatVal;

    // if the attribute is an integer, convert the string value to a binary int
    if (type == INTEGER) {
      intVal = atoi(attrValue);
      filter = (char *)&intVal;
    // if the attribute is a float, convert the string value to a binary float
    } else if (type == FLOAT) {
      floatVal = atof(attrValue);
      filter = (char *)&floatVal;
    }

    // start a conditional scan using the attribute's offset, length, type, filter value, and operator
    status =
        scan.startScan(attrDesc.attrOffset, attrDesc.attrLen, type, filter, op);
  }

  // if starting the scan failed, return the error
  if (status != OK)
    return status;

  // rid will hold the record id of each matching record found during the scan
  RID rid;
  // scan through all matching records one by one
  while (scan.scanNext(rid) == OK) {
    // delete the current matching record
    status = scan.deleteRecord();
    // if deletion failed, return the error immediately
    if (status != OK)
      return status;
  }

  // all matching records deleted successfully
  return OK;
}