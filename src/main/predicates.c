/*******************************************************************************
 * Copyright 2013-2015 Aerospike, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

/**
 * from aerospike import predicates as p
 *
 * q = client.query(ns,set).where(p.equals("bin",1))
 */

#include <Python.h>
#include <aerospike/as_query.h>
#include <aerospike/as_error.h>

#include "conversions.h"
#include "exceptions.h"
#include "geo.h"

static PyObject * AerospikePredicates_Equals(PyObject * self, PyObject * args)
{
	PyObject * py_bin = NULL;
	PyObject * py_val = NULL;

	if (PyArg_ParseTuple(args, "OO:equals", 
			&py_bin, &py_val) == false) {
		goto exit;
	}

	if (PyInt_Check(py_val) || PyLong_Check(py_val)) {
		return Py_BuildValue("iiOO", AS_PREDICATE_EQUAL, AS_INDEX_NUMERIC, py_bin, py_val);
	} else if (PyString_Check(py_val) || PyUnicode_Check(py_val)) {
		return Py_BuildValue("iiOO", AS_PREDICATE_EQUAL, AS_INDEX_STRING, py_bin, py_val);
	}

exit:
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * AerospikePredicates_Contains(PyObject * self, PyObject * args)
{
	PyObject * py_bin = NULL;
	PyObject * py_indextype = NULL;
	PyObject * py_val = NULL;
	int index_type;

	if (PyArg_ParseTuple(args, "OOO:equals", 
				&py_bin, &py_indextype, &py_val) == false) {
		goto exit;
	}

	if (PyInt_Check(py_indextype)) {
		index_type = PyInt_AsLong(py_indextype);
	} else if (PyLong_Check(py_indextype)) {
		index_type = PyLong_AsLongLong(py_indextype);
	} else {
		goto exit;
	}

	if (PyInt_Check(py_val) || PyLong_Check(py_val)) {
		return Py_BuildValue("iiOOOi", AS_PREDICATE_EQUAL, AS_INDEX_NUMERIC, py_bin, py_val, Py_None, index_type);
	} else if (PyString_Check(py_val) || PyUnicode_Check(py_val)) {
		return Py_BuildValue("iiOOOi", AS_PREDICATE_EQUAL, AS_INDEX_STRING, py_bin, py_val, Py_None, index_type);
	}

exit:
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * AerospikePredicates_RangeContains(PyObject * self, PyObject * args)
{
	PyObject * py_bin = NULL;
	PyObject * py_indextype = NULL;
	PyObject * py_min = NULL;
	PyObject * py_max= NULL;
	int index_type;

	if (PyArg_ParseTuple(args, "OOOO:equals",
			&py_bin, &py_indextype, &py_min, &py_max) == false) {
		goto exit;
	}

	if (PyInt_Check(py_indextype)) {
		index_type = PyInt_AsLong(py_indextype);
	} else if (PyLong_Check(py_indextype)) {
		index_type = PyLong_AsLongLong(py_indextype);
	} else {
		goto exit;
	}

	if ((PyInt_Check(py_min) || PyLong_Check(py_min)) && (PyInt_Check(py_max) || PyLong_Check(py_max))) {
		return Py_BuildValue("iiOOOi", AS_PREDICATE_RANGE, AS_INDEX_NUMERIC, py_bin, py_min, py_max, index_type);
	}

exit:
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * AerospikePredicates_Between(PyObject * self, PyObject * args)
{
	PyObject * py_bin = NULL;
	PyObject * py_min = NULL;
	PyObject * py_max = NULL;

	if (PyArg_ParseTuple(args, "OOO:between",
			&py_bin, &py_min, &py_max) == false) {
		goto exit;
	}

	if ((PyInt_Check(py_min) || PyLong_Check(py_min)) && (PyInt_Check(py_max) || PyLong_Check(py_max))) {
		return Py_BuildValue("iiOOO", AS_PREDICATE_RANGE, AS_INDEX_NUMERIC, py_bin, py_min, py_max);
	}

exit:
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * AerospikePredicates_GeoWithin_GeoJSONRegion(PyObject * self, PyObject * args)
{
	PyObject * py_bin = NULL;
	PyObject * py_shape = NULL;

	if (PyArg_ParseTuple(args, "OO:geo_within_geojson_region",
			&py_bin, &py_shape) == false) {
		goto exit;
	}

	if (PyString_Check(py_shape) || PyUnicode_Check(py_shape)) {
		return Py_BuildValue("iiOO", AS_PREDICATE_RANGE, AS_INDEX_GEO2DSPHERE, py_bin, py_shape);
	}

exit:
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * AerospikePredicates_GeoWithin_Radius(PyObject * self, PyObject * args)
{
	PyObject * py_bin = NULL;
	PyObject * py_lat = NULL;
	PyObject * py_long = NULL;
	PyObject * py_radius = NULL;
	PyObject * py_geo_object = NULL;
	PyObject * py_shape = NULL;

	as_error err;
	as_error_init(&err);

	py_geo_object = PyDict_New();

	if (PyArg_ParseTuple(args, "OOOO:geo_within_radius", 
			&py_bin, &py_lat, &py_long, &py_radius) == false) {
		goto CLEANUP;
	}

	PyObject *py_circle = PyString_FromString("AeroCircle");
	PyDict_SetItemString(py_geo_object, "type", py_circle);
	Py_DECREF(py_circle);

	if (PyString_Check(py_bin) && PyFloat_Check(py_lat) && PyFloat_Check(py_long) && PyFloat_Check(py_radius)) {
		PyObject * py_outer_list = PyList_New(2);
		PyObject * py_inner_list = PyList_New(2);
		PyList_SetItem(py_inner_list, 0, py_lat);
		PyList_SetItem(py_inner_list, 1, py_long);

		PyList_SetItem(py_outer_list, 0, py_inner_list);
		PyList_SetItem(py_outer_list, 1, py_radius);

		PyDict_SetItemString(py_geo_object, "coordinates", py_outer_list);

		py_shape = AerospikeGeospatial_DoDumps(py_geo_object, &err);

		if (!py_shape) {
			as_error_update(&err, AEROSPIKE_ERR_CLIENT, "Unable to call dumps function");
			goto CLEANUP;
		}
	} else {
		as_error_update(&err, AEROSPIKE_ERR_PARAM, "Latitude, longitude and radius should be of double type, bin of string type");
		goto CLEANUP;
	}
	
	return Py_BuildValue("iiOO", AS_PREDICATE_RANGE, AS_INDEX_GEO2DSPHERE, py_bin, py_shape);

CLEANUP:
	// If an error occurred, tell Python.
	if (err.code != AEROSPIKE_OK) {
		PyObject * py_err = NULL;
		error_to_pyobject(&err, &py_err);
		PyObject *exception_type = raise_exception(&err);
		PyErr_SetObject(exception_type, py_err);
		Py_DECREF(py_err);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * AerospikePredicates_GeoContains_GeoJSONPoint(PyObject * self, PyObject * args)
{
	PyObject * py_bin = NULL;
	PyObject * py_point = NULL;

	if (PyArg_ParseTuple(args, "OO:geo_contains_geojson_point", &py_bin, &py_point) == false) {
		goto exit;
	}

	if (PyString_Check(py_point) || PyUnicode_Check(py_point)) {
		return Py_BuildValue("iiOOi", AS_PREDICATE_RANGE, AS_INDEX_GEO2DSPHERE, py_bin, py_point, 1);
	}

exit:
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * AerospikePredicates_GeoContains_Point(PyObject * self, PyObject * args)
{
	PyObject * py_bin = NULL;
	PyObject * py_lat = NULL;
	PyObject * py_long = NULL;
	PyObject * py_geo_object = NULL;
	PyObject * py_shape = NULL;

	as_error err;
	as_error_init(&err);

	py_geo_object = PyDict_New();

	if (PyArg_ParseTuple(args, "OOO:geo_contains_point", 
			&py_bin, &py_lat, &py_long) == false) {
		goto CLEANUP;
	}

	PyObject *py_point = PyString_FromString("Point");
	PyDict_SetItemString(py_geo_object, "type", py_point);
	Py_DECREF(py_point);

	if (PyString_Check(py_bin) && PyFloat_Check(py_lat) && PyFloat_Check(py_long)) {
		PyObject * py_list = PyList_New(2);
		PyList_SetItem(py_list, 0, py_lat);
		PyList_SetItem(py_list, 1, py_long);

		PyDict_SetItemString(py_geo_object, "coordinates", py_list);

		py_shape = AerospikeGeospatial_DoDumps(py_geo_object, &err);

		if (!py_shape) {
			as_error_update(&err, AEROSPIKE_ERR_CLIENT, "Unable to call dumps function");
			goto CLEANUP;
		}
	} else {
		as_error_update(&err, AEROSPIKE_ERR_PARAM, "Latitude and longitude should be of double type, bin of string type");
		goto CLEANUP;
	}
	
	return Py_BuildValue("iiOOi", AS_PREDICATE_RANGE, AS_INDEX_GEO2DSPHERE, py_bin, py_shape, 1);

CLEANUP:
	// If an error occurred, tell Python.
	if (err.code != AEROSPIKE_OK) {
		PyObject * py_err = NULL;
		error_to_pyobject(&err, &py_err);
		PyObject *exception_type = raise_exception(&err);
		PyErr_SetObject(exception_type, py_err);
		Py_DECREF(py_err);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef AerospikePredicates_Methods[] = {
	{"equals",		(PyCFunction) AerospikePredicates_Equals,	METH_VARARGS, "Tests whether a bin's value equals the specified value."},
	{"between",		(PyCFunction) AerospikePredicates_Between,	METH_VARARGS, "Tests whether a bin's value is within the specified range."},
	{"contains",	(PyCFunction) AerospikePredicates_Contains,	METH_VARARGS, "Tests whether a bin's value equals the specified value in a complex data type"},
	{"range",	(PyCFunction) AerospikePredicates_RangeContains,	METH_VARARGS, "Tests whether a bin's value is within the specified range in a complex data type"},
	{"geo_within_geojson_region",		(PyCFunction) AerospikePredicates_GeoWithin_GeoJSONRegion,	METH_VARARGS, "Tests whether a bin's value is within the specified shape."},
	{"geo_within_radius",		(PyCFunction) AerospikePredicates_GeoWithin_Radius,	METH_VARARGS, "Create a geo_within_geojson_region predicate"},
	{"geo_contains_geojson_point",		(PyCFunction) AerospikePredicates_GeoContains_GeoJSONPoint,	METH_VARARGS, "Tests whether a bin's value contains the specified point."},
	{"geo_contains_point",		(PyCFunction) AerospikePredicates_GeoContains_Point,	METH_VARARGS, "Create a geo_contains_geojson_point predicate"},
	{NULL, NULL, 0, NULL}
};


PyObject * AerospikePredicates_New(void)
{
	PyObject * module = Py_InitModule3("aerospike.predicates", AerospikePredicates_Methods, "Query Predicates");
	return module;
}
