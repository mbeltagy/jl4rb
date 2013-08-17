/**********************************************************************

  jl4rb.c

**********************************************************************/
#include <stdio.h>
#include <string.h>
 
#include "julia-api.h"
//-| next macros already exist in ruby and are undefined here
#undef T_FLOAT
#undef NORETURN
#include "ruby.h"
/************* INIT *********************/


VALUE Julia_init(VALUE obj, VALUE args)
{
  char **argv,*julia_home_dir,*mode;
  int i,argc;
  VALUE tmp;

  argc=RARRAY_LEN(args) + 1;
  tmp=rb_ary_entry(args,0);
  julia_home_dir=StringValuePtr(tmp);
  tmp=rb_ary_entry(args,1);
  mode=StringValuePtr(tmp);
  //printf("First initialization with julia_home_dir=%s\n",julia_home_dir);

#ifdef WITH_JULIA_RELEASE
  jl_init(julia_home_dir);
#else 
  jlapi_init(julia_home_dir,mode);
#endif
 
  return Qtrue;
}

//Maybe try to use cpp stuff to get the output inside julia system (ccall,cgen and cgutils)
//-| TODO: after adding in the jlapi.c jl_is_<C_type> functions replace the strcmp! 
VALUE jl_value_to_VALUE(jl_value_t *res) {
  size_t i=0,k,nd,d;
  VALUE resRb;
  jl_value_t *tmp;
  jl_function_t *call;

  if(res!=NULL) { //=> get a result
    //printf("typeof=%s\n",jl_typeof_str(res));
    if(strcmp(jl_typeof_str(res),"Int64")==0 || strcmp(jl_typeof_str(res),"Int32")==0) 
    //if(jl_is_long(res)) //does not work because of DLLEXPORT
    {
      //printf("elt=%d\n",jl_unbox_long(res));
      return INT2FIX(jl_unbox_long(res));
    }
    else
    if(strcmp(jl_typeof_str(res),"Float64")==0)
    //if(jl_is_float64(res)) 
    {
      return rb_float_new(jl_unbox_float64(res));
    }
    else
    if(strcmp(jl_typeof_str(res),"Float32")==0)
    //if(jl_is_float64(res)) 
    {
      return rb_float_new(jl_unbox_float32(res));
    }
    else 
    if(strcmp(jl_typeof_str(res),"Bool")==0)
    //if(jl_is_bool(res)) 
    {
      return (jl_unbox_bool(res) ? Qtrue : Qfalse);
    }
    else 
    if(strcmp(jl_typeof_str(res),"DataType")==0)
    //if(jl_is_bool(res)) 
    {
      return rb_str_new2(jl_typename_str(res));
    }
    else 
    if(strcmp(jl_typeof_str(res),"Nothing")==0)
    //if(jl_is_bool(res)) 
    {
      return Qnil;
    }
    else
    if(strcmp(jl_typeof_str(res),"Complex")==0)
    //if(jl_is_bool(res)) 
    {
      resRb = rb_eval_string("require('complex');Complex.new(0,0)");
      rb_iv_set(resRb,"@real",jl_value_to_VALUE(jl_get_field(res, "re")));
      rb_iv_set(resRb,"@image",jl_value_to_VALUE(jl_get_field(res, "im")));
      return resRb;
    }
    else
    if(strcmp(jl_typeof_str(res),"Regex")==0)
    //if(jl_is_bool(res)) 
    {
      // call=(jl_function_t*)jl_get_global(jl_base_module, jl_symbol("show"));
      // printf("ici\n");
      // if (call) tmp=jl_call1(call,res);
      // else printf("call failed!\n");
      // printf("ici\n");
      resRb = jl_value_to_VALUE(jl_get_field(res, "pattern"));
      return resRb;
    }
    else
    if(strcmp(jl_typeof_str(res),"ASCIIString")==0 || strcmp(jl_typeof_str(res),"UTF8String")==0)
    {
      //printf("value=%s\n",jl_bytestring_ptr(res));
      return rb_str_new2(jl_bytestring_ptr(res));
    }
    else
    if(strcmp(jl_typeof_str(res),"Array")==0 )
    //if(jl_is_array(res))
    {
      nd = jl_array_rank(res);
      //printf("array_ndims=%d\n",(int)nd);
      if(nd==1) {//Vector
        d = jl_array_size(res, 0);
        //printf("array_dim[1]=%d\n",(int)d);
        resRb = rb_ary_new2(d);
        for(i=0;i<d;i++) {
          rb_ary_store(resRb,i,jl_value_to_VALUE(jl_arrayref((jl_array_t *)res,i)));
        }
        return resRb;
      }
      //TODO: multidim array ruby equivalent???? Is it necessary 
      
    }
    else
    if(strcmp(jl_typeof_str(res),"Tuple")==0 )
    //if(jl_is_array(res))
    {
      d=jl_tuple_len(res);
      resRb = rb_ary_new2(d);
      for(i=0;i<d;i++) {
        rb_ary_store(resRb,i,jl_value_to_VALUE(jl_tupleref(res,i)));
      }
      return resRb;
    }
    resRb=rb_str_new2("__unconverted(");
    rb_str_cat2(resRb, jl_typeof_str(res));
    rb_str_cat2(resRb, ")__\n");
    printf("%s\n",jl_bytestring_ptr(jl_eval_string("\"$(ans)\"")));
    // jl_function_t *call=(jl_function_t*)jl_get_global(jl_base_module, jl_symbol("show"));
    // if (call) jl_call1(call,res);
    // else printf("call failed!\n");

    return resRb;
  }
  //=> No result (command incomplete or syntax error)
#ifndef WITH_JULIA_RELEASE
  jlapi_print_stderr(); //If this happens but this is really not sure!
#endif
  resRb=rb_str_new2("__incomplete");
  if(jl_exception_occurred()!=NULL) {
    rb_str_cat2(resRb, "(");
      rb_str_cat2(resRb,jl_typeof_str(jl_exception_occurred()));
    jl_value_t* err=jl_get_field(jl_exception_occurred(),"msg");
    if(err!=NULL) printf("%s: %s\n",jl_typeof_str(jl_exception_occurred()),jl_bytestring_ptr(err));
    jl_exception_clear();
    rb_str_cat2(resRb, ")");
  }
  rb_str_cat2(resRb, "__");
  return resRb;
}

/***************** EVAL **********************/

VALUE Julia_eval(VALUE obj, VALUE cmd, VALUE print_stdout)
{
  char *cmdString;
  jl_value_t *res;
   
  cmdString=StringValuePtr(cmd);
#ifndef WITH_JULIA_RELEASE
  //This flush redirected stdout before printing
  if(print_stdout!=Qnil) jlapi_get_stdout();
#endif
  res=jl_eval_string(cmdString);
  jl_set_global(jl_base_module, jl_symbol("ans"),res);
#ifndef WITH_JULIA_RELEASE
  if(print_stdout!=Qnil) jlapi_print_stdout();
#endif
  return jl_value_to_VALUE(res);
}

VALUE Julia_exec(VALUE obj, VALUE cmd, VALUE get_stdout)
{
  char *cmdString,*outString;
  jl_value_t *res;
  VALUE out;
   
  cmdString=StringValuePtr(cmd);
  res=jl_eval_string(cmdString);
  jl_set_global(jl_base_module, jl_symbol("ans"),res);
  if(get_stdout!=Qnil) {
    outString=jlapi_get_stdout();
    jl_set_global(jl_base_module, jl_symbol("rbout"),jl_cstr_to_string(outString));
    return  rb_str_new2(outString);
  } else return Qnil;
}

void
Init_jl4rb()
{
  VALUE mJulia;

  mJulia = rb_define_module("Julia");

  rb_define_module_function(mJulia, "initJL", Julia_init, 1);

  rb_define_module_function(mJulia, "evalLine", Julia_eval, 2);

  rb_define_module_function(mJulia, "execLine", Julia_exec, 2);

}
