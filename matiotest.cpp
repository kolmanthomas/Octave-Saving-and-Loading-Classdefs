#include <octave/oct.h>

#include "octave/ov-re-mat.h"

#include <octave/cdef-manager.h>
#include <octave/cdef-utils.h>
#include <octave/interpreter.h>

#include <octave/stack-frame.h>

#include <matio.h>

// Globals
octave::interpreter* interp = octave::interpreter::the_interpreter();
octave::cdef_manager& cdef_mgr = interp->get_cdef_manager();
octave::tree_evaluator& eval = interp->get_evaluator();


template <typename T>
Array<T> read_data(const matvar_t *matvar)
{
    dim_vector dv;
    dv.alloc (matvar->rank);
    for (int i = 0; i < matvar->rank; ++i) {
        dv(i) = matvar->dims[i];  
    }

    // data will be freed by Array destructor
    T *data = new T[matvar->nbytes / matvar->data_size];
    octave_stdout << "Reading data of size: " << matvar->nbytes << " bytes, with data size: " << matvar->data_size << " bytes.\n";
    std::memcpy(data, matvar->data, matvar->nbytes);

    Array<T> a (data, dv); 
    return std::move(a);
}

// Since we have many types we could possibly return here, we'll just instead pass the struct as a parameter to be filled
void get_var(const matvar_t *matvar, octave_scalar_map& st)
{
    switch (matvar->class_type) {
        case MAT_C_DOUBLE: {
            Array<double> a = read_data<double>(matvar);
            NDArray var (a);
            st.assign(matvar->name, var);
            break;
        }
        case MAT_C_SINGLE: {
            octave_stdout << "SINGLE\n";
            octave_stdout << "Variable " << matvar->name << " is of type single.\n";
            break;
        }
        case MAT_C_INT8:
            octave_stdout << "INT8\n";
            octave_stdout << "Variable " << matvar->name << " is of type int8.\n";
            break;
        case MAT_C_UINT8:
            octave_stdout << "UINT8\n";
            octave_stdout << "Variable " << matvar->name << " is of type uint8.\n";
            break;
        case MAT_C_INT16:
            octave_stdout << "INT16\n";
            octave_stdout << "Variable " << matvar->name << " is of type int16.\n";
            break;
        case MAT_C_UINT16:
            octave_stdout << "UINT16\n";
            octave_stdout << "Variable " << matvar->name << " is of type uint16.\n";
            break;
        case MAT_C_INT32:
            octave_stdout << "INT32\n";
            octave_stdout << "Variable " << matvar->name << " is of type int32.\n";
            break;
        case MAT_C_UINT32:
            octave_stdout << "UINT32\n";
            octave_stdout << "Variable " << matvar->name << " is of type uint32.\n";
            break;
        case MAT_C_INT64:
            octave_stdout << "INT64\n";
            octave_stdout << "Variable " << matvar->name << " is of type int64.\n";
            break;
        case MAT_C_UINT64:
            octave_stdout << "UINT64\n";
            octave_stdout << "Variable " << matvar->name << " is of type uint64.\n";
            break;
        case MAT_C_CHAR: {
            octave_stdout << "CHAR\n";
            charNDArray a (read_data<char>(matvar));
            st.assign(matvar->name, a);
            break;
        }
        case MAT_C_CELL:
            octave_stdout << "CELL\n";
            octave_stdout << "Variable " << matvar->name << " is a cell array.\n";
            break;
        case MAT_C_STRUCT: {
            octave_stdout << "STRUCT\n";
            // Get the number of fields in the struct
            //unsigned int num_fields = Mat_VarGetNumberOfFields(matvar);
            //matvar_t *fields = Mat_VarGetStructsLinear(matvar, 0, 1, num_fields, 0);


                
            //Mat_VarPrint(&fields[0], 1);

            //Mat_VarFree(fields);
            break;
        }
        default:
            octave_stdout << "DEFAULT\n";
            octave_stdout << "Variable "
                          << matvar->name
                          << ": Unknown class type "
                          << ".\n";
    }
}

void readclass(const std::string& filename, octave_scalar_map& st)
{
    mat_t *matfp = Mat_Open (filename.c_str(), MAT_ACC_RDONLY);

    if (matfp == NULL) {
        error("matiotest: could not open file");
    }

#ifndef NDEBUG
    octave_stdout << "Header: " << Mat_GetHeader(matfp) << "\n";
#endif

    // Iterate through the variables in the MAT file
    matvar_t *matvar; 
    while ( (matvar = Mat_VarReadNext(matfp)) != NULL) {


#ifndef NDEBUG
        octave_stdout << "===== Variable data =========\n";
        Mat_VarPrint(matvar, 0);
        octave_stdout << "===== END Variable data =====\n";
#endif

        // std::string does a deep copy
        std::string name (matvar->name);

        get_var(matvar, st); 

        octave_stdout << "Variable '" << name << "' has been read.\n";

        Mat_VarFree(matvar);
    }

#ifndef NDEBUG
    octave_stdout << "===== Contents of struct ========\n";
    for (auto it = st.begin(); it != st.end(); ++it) {
        octave_stdout << "Field '" << it->first << "' has value: ";
        st.getfield(it->first).short_disp(octave_stdout);
        octave_stdout << "\n";
    }
    octave_stdout << "===== END Contents of struct ====\n";
#endif

    Mat_Close(matfp);
}

octave_value_list
loadobj(const octave_scalar_map& st,
        //octave::cdef_object& obj,
        octave_value obj, // this is the cdef_object that is constructed.
        octave::cdef_method& loadobj_method)
{
    octave_value_list args;
    octave_value_list retval;

    // TODO: Instead of forcing 'obj' to be a parameters, try to push object variable onto the stack frame of the call

    /*
        // In the scope of the method, we'll add a default-constructed object
        // Maybe in the global scope?
        //octave::interpreter* interp = octave::interpreter::the_interpreter();
        //octave::tree_evaluator& eval = interp->get_evaluator();
        //interp->global_assign("abc", octave_value(1));

        octave::tree_evaluator& eval = interp->get_evaluator();
        eval.top_level_assign("abc", octave_value(1));

        auto names = interp->top_level_variable_names();
        for (auto& name : names) {
            octave_stdout << "Global variable: " << name << "\n";
        }
        //interp->set_global_value("abc", octave_value(2));

        octave_stdout << "Stack frame num: " << eval.current_call_stack_frame_number() << "\n";;

        // We pass in a struct, and expect a new object to be created

        octave_stdout << "Stack frame num: " << eval.current_call_stack_frame_number() << "\n";;
        octave::unwind_protect up; 

        octave::symbol_scope ss = eval.get_current_scope();

        if (ss.is_variable("abc")) {
            octave_stdout << "abc found in symbol table.\n";
        } else {
            octave_stdout << "abc not found in symbol table.\n";
        }

        octave::symbol_scope ss_top = interp->get_top_scope();
        if (ss_top.is_variable("abc")) {
            octave_stdout << "abc found in symbol table.\n";
        } else {
            octave_stdout << "abc not found in symbol table.\n";
        }
        //obj.set_property (1, "data", retval(0));
    */


    args(0) = octave_value(obj);
    args(1) = octave_value(st);
    retval = loadobj_method.execute (args, 1);

    return retval;
}

octave_scalar_map saveobj(
    octave_value obj, // this is the cdef_object 
    octave::cdef_method& loadobj_method)
{
    octave_value_list args;
    args(0) = obj; 
    octave_value_list retval = loadobj_method.execute(obj, 1);

    if (!retval(0).is_defined() || !retval(0).isstruct()) {
        error("matiotest: saveobj method did not return a scalar map.");
    }

    octave_scalar_map st = retval(0).scalar_map_value();
    
    return st;
}

matvar_t*
write_var(const std::string& name, octave_value val)
{
    dim_vector dims_v = val.dims();
    int rank = dims_v.ndims();

    size_t *dims = (size_t*) &dims_v.elem(0); // Guaranteed that index elem 0 exists
    
    matvar_t *matvar = NULL;

    octave_stdout << "How many times for " << val.type_id() << "?\n";
    switch (val.type_id()) {
        case 0: { // bytp_double
            octave_stdout << "is a double scalar\n";
            break;
        }
        case 1: { // btyp_float
            octave_stdout << "is a scalar\n";
            // Get the underlying pointer
            break;
        }
        case 12: { // btyp_char
            octave_stdout << "is a char array\n";
            // Get the underlying pointer
            const char* data = val.char_array_value().data();
                matvar = Mat_VarCreate (name.c_str(), MAT_C_CHAR, MAT_T_UTF8, rank, dims, data, 0);
            break;
        }
        case 13: { // btyp_struct
            octave_stdout << "is a struct\n";
            break;
        }
        case 14: { // btyp_cell
            octave_stdout << "is a cell array\n";
            break;
        }
    }
    octave_stdout << "never, apparently?\n";

    return matvar;
}

void 
writeclass(const std::string& filename,
           octave_scalar_map st)
{
    mat_t *matfp = Mat_CreateVer (filename.c_str(), NULL, MAT_FT_MAT73);

    if (matfp == NULL) {
        error("matiotest: could not create file");
    }

    for (auto it = st.begin(); it != st.end(); ++it) {
        octave_stdout << "property '" << it->first << "' has value: " << st.getfield(it->first).string_value() << "\n";

        dim_vector dims_v = st.getfield(it->first).dims();
        int rank = dims_v.ndims();

        size_t *dims = (size_t*) &dims_v.elem(0); // Guaranteed that index elem 0 exists
        // We've gotta convert dims to a const size_t* pointer ourselves; it's ugly, but needs to be done, since the pointer is private in dim_vector
        // size_t *dims = new size_t[rank];
        
        matvar_t *matvar = write_var(it->first, st.getfield(it->first));
        if (matvar == NULL) {
            error("matiotest: could not create matvar_t object for variable '%s'", it->first.c_str());
        }

        Mat_VarWrite (matfp, matvar, MAT_COMPRESSION_NONE);

        Mat_VarFree(matvar);
    }

    Mat_Close(matfp);

    octave_stdout << "====== TEST =========" << "\n";
    const char* hello = "Hello, world!"; // 13 characters
    octave_stdout << "Size of string: " << strlen(hello) << "\n";
    octave_stdout << "====== END TEST ===== " << "\n";

    // Let's verify that the file was created with all the right variables
    octave_scalar_map st2;
    readclass(filename, st2);
}

//
//
// Parameters:
//  arg0: 'r' or 'w'
//  arg1: filename
//  arg2: classname (if loading), or object (if saving)
DEFUN_DLD (matiotest, args, nargout,
           "MATLAB io save method")
{
    octave_idx_type nargin = args.length ();

    // We're going to check that all the parameters are valid before we start loading or saving anything.

    if (nargin < 3 || nargin > 3) {
        error ("hdf5class: Invalid number of input arguments. Expected 2.");
    }

    if (!(args(0).is_defined() && args(0).is_string())) {
        error ("hdf5class: First argument must be a string, either 'r' or 'w'."); 
    }

    std::string opt = args(0).string_value ();

    octave_value obj;
    octave::cdef_method loadobj_method;
    octave::cdef_method saveobj_method;

    octave_value_list retval (nargout);
    if (opt == "r") {
        if (!(args(1).is_defined() && args(1).is_string())) {
            error ("matiotest: Second argument must be a string for a filename to read from."); 
        }
        std::string filename = args(1).string_value();

        if (args(2).is_defined() && args(2).is_string()) {
            // Check to see if the string represents a class name
            octave::cdef_class cls = cdef_mgr.find_class(args(2).string_value());

            if (!cls.is_class()) {
                error("matiotest: Class not found: ");
            } else {
                octave_stdout << "Found class: " << cls.get_name() << "\n";
            }

            // We do a check to see if the class has a loadobj method
            //octave_value loadobj_method = cls.get_method_function("loadobj");
            std::map<std::string, octave::cdef_method> m = cls.get_method_map();

            if (m.find("loadobj") == m.end()) {
                error("matiotest: Class does not have a loadobj method.");
            }

            // Verify that loadobj is a static method
            if (!m["loadobj"].is_static()) {
                error("matiotest: Class 'loadobj' method is not static.");
            }

            loadobj_method = m["loadobj"];

            /*
            octave_fcn_handle *loadobj_method_handle = loadobj_method.fcn_handle_value();
            if (loadobj_method_handle->is_defined() && loadobj_method_handle->is_function_handle()) {
                octave_stdout << "has a loadobj method.\n";
            } else {
                error("matiotest: Class does not have a loadobj method.");
            }
            */
    
            // This snippet doesn't work (why?)
            //     cls.initialize_object(obj);
            //     out = octave::to_ov(obj);
            
            // We're gonna just assume that ConstructOnLoad = true for now
            obj = cls.construct(octave_value_list());
        } else {
            error("matiotest: If \"r\" is specified, third argument must be a valid class name.");
        }

        octave_stdout << "Reading from MAT file: " << filename << "\n";
        octave_scalar_map st;
        readclass(filename, st);

        retval = loadobj(st, obj, loadobj_method);
    } else if (opt == "w") {
        octave_stdout << "Writing to MAT file.\n";

        // We'll test if we can actually open the file later
        if (!(args(1).is_defined() && args(1).is_string())) {
            error ("matiotest: Second argument must be a string for a filename to write to."); 
        }
        std::string filename = args(1).string_value();

        // Name of object to get
        if (args(2).is_defined() && args(2).is_string()) {
            // Reach into the interpreter to get the object
            obj = interp->find("m");

            if (!(obj.is_classdef_object())) {
                error("matiotest: Third argument must be a classdef object.");
            }

            // Make sure that obj has a loadobj method, get a handle to it
            // We'll find the class name and see if the 'loadobj' method exists, and then get the handle from it 
            octave::cdef_class cls = cdef_mgr.find_class(obj.class_name());

            std::map<std::string, octave::cdef_method> m = cls.get_method_map();
            if (m.find("saveobj") == m.end()) {
                error("matiotest: Class does not have a saveobj method.");
            }

            if (m["saveobj"].is_static()) {
                error("matiotest: 'saveobj' method should not be static.");
            }

            saveobj_method = m["saveobj"];

            octave_scalar_map st = saveobj(obj, m["saveobj"]); 

            // Now we have a struct, we can write it to the MAT file

            writeclass(filename, st);
        
            retval(0) = octave_value(1);
        }
    } else {
        error ("hdf5class: First argument must be 'r' or 'w'.");
    }


    /*
    for (int i = 0; i < nargout; i++)
        retval(i) = octave_value(5);
    */
    //retval(0) = out;

    octave_stdout << "=====END OF MATIO TEST=====\n";
    return retval;
}



