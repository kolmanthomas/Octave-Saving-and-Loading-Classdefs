#include <octave/oct.h>
#include <octave/cdef-class.h>
#include <octave/cdef-manager.h>
#include <octave/cdef-utils.h>
#include <octave/cdef-object.h>

#include <octave/ov-struct.h>

#include <octave/ls-oct-text.h>

#include <octave/interpreter.h>

#include <iostream>

// args: octave_value_list
// Return type of DLD is always octave_value_list
DEFUN_DLD (saveclass, args, nargout,
           "Classdef save method")
{
    octave_idx_type nargin = args.length();

    if (nargin < 1) {
        error("saveclass: at least one input argument is required.");
    } else if (nargin > 2) {
        error("saveclass: too many input arguments.");
    }

    // Check if the first argument is a classdef object
    if (!args(0).is_defined() || !args(0).is_classdef_object()) {
        error("saveclass: first argument must be a classdef object.");
    } 
    octave::cdef_object obj = octave::to_cdef(args(0));

    // Check if the second argument is a string for a valid filename.
    // If no second argument, then use the same name as the class.
    std::string filename = obj.class_name();
    if (nargin == 2) {
        if (!args(1).is_string()) {
            error("saveclass: second argument must be a string.");
        }
        filename = args(1).string_value();
    }

    std::ofstream outf(filename);
    if (!outf) {
        error("saveclass: could not open file for writing.");
    }

    // Get the class properties
    std::map<octave::property_key, octave::cdef_property> properties = obj.get_class().get_property_map();

    // Save all the values to a struct
    octave_scalar_map st;
    for (const auto& property : properties) {
        // Get the property name
        std::string property_name = property.first.second;
        octave_value property_value = obj.get_property(0, property_name);
        octave_stdout << "Property: " << property_name << " has property value: " << property_value.string_value() << "\n";
        st.assign(property_name, property_value);
    }

    save_text_data(outf, st, filename, true, 0);


    // Return empty matrices for any outputs
    octave_value_list retval (nargout);
    for (int i = 0; i < nargout; i++)
        retval(i) = octave_value (Matrix ());

    return retval;
}

/*
 * %!test
 * %! assert (saveclass)
 *
 */

