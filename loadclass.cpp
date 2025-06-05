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
DEFUN_DLD (loadclass, args, nargout,
           "Classdef load method")
{
    octave_idx_type nargin = args.length();

    if (nargin < 1) {
        error("loadclass: at least one input argument is required.");
    } else if (nargin > 2) {
        error("loadclass: too many input arguments.");
    }

    // Check if the first argument is a filename of a classdef object 
    if (!args(0).is_defined() || !args(0).is_string()) {
        error("loadclass: first argument must be a filename.");
    } 
    const std::string filename = args(0).string_value();

    // We'll check the second argument later

    std::ifstream file (filename);
    if (!file) {
        error("loadclass: could not open file for reading.");
    }

    octave_value tc;
    bool global = false;
    std::string txt = read_text_data(file, filename, global, tc, 0);

    octave_stdout << "Loaded class definition from file: " << txt << "\n";

    if (!tc.isstruct()) {
        error("loadclass: loaded data is not a struct.");
    }
    octave_scalar_map st = tc.scalar_map_value();
    
    // Let's save everything to a struct
    for (const auto& entry : st) {
        // Print the property name and value
        octave_stdout << "Property: " << entry.first << " has value: " 
                      << st.contents(entry.second).string_value() << "\n";
    }

    octave_value_list val_list(1);
    val_list(0) = st;

    // Get the interpreter and load/save system
    octave::interpreter* interp = octave::interpreter::the_interpreter();
    octave::cdef_manager& cdef_mgr = interp->get_cdef_manager();
    octave::cdef_class cls = cdef_mgr.find_class(filename);

    // For now, we assume the constructor takes in a struct
    octave::cdef_object obj = cls.construct_object(val_list);


    // How do I return the object in an octave_value? Beats me
    octave_value_list retval (nargout);
    //octave_value val_obj = obj.get();
    //retval(0) = obj; 

    return retval;
}


