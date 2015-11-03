#include <stdio.h>
#include <jni.h>

JNIEnv* create_vm(JavaVM **jvm)
{
    JNIEnv* env;
    JavaVMInitArgs args;
    JavaVMOption options;
    args.version = JNI_VERSION_1_6;
    args.nOptions = 1;
    options.optionString = "-Djava.class.path=./";
    args.options = &options;
    args.ignoreUnrecognized = 0;
    int rv;
    rv = JNI_CreateJavaVM(jvm, (void**)&env, &args);
    if (rv < 0 || !env)
        printf("Unable to Launch JVM %d\n",rv);
    else
        printf("Launched JVM! :)\n");
    return env;
}

void invoke_class(JNIEnv* env)
{
    //class creation
    jclass handler_class;jclass request_class;jclass response_class;
    handler_class = (*env)->FindClass(env, "Handler");
    request_class = (*env)->FindClass(env, "Request");
    response_class = (*env)->FindClass(env, "Response");
    //end of class creation

    //constructor creation
    jmethodID handler_constructor;jmethodID request_constructor;jmethodID response_constructor;
    handler_constructor = (*env)->GetMethodID(env,handler_class, "<init>", "()V");
    request_constructor = (*env)->GetMethodID(env,request_class, "<init>", "()V");
    response_constructor = (*env)->GetMethodID(env,response_class, "<init>", "()V");
    //end of constructor creation

    //object creation
    jobject handler_object =  (*env)->NewObject(env,handler_class, handler_constructor);
    jobject request_object =  (*env)->NewObject(env,request_class, request_constructor);
    jobject response_object =  (*env)->NewObject(env,response_class, response_constructor);
    //end of object creation

    //method creation
    jmethodID first_method = (*env)->GetMethodID(env, handler_class, "first", "(LRequest;LResponse;)V");
    jmethodID set_request_body_method = (*env)->GetMethodID(env, request_class, "setRequestBody", "(Ljava/lang/String;)V");
    jmethodID get_response_body_method = (*env)->GetMethodID(env, response_class, "getResponseBody", "()Ljava/lang/String;");
    //end of method creaion

    char ht[512];
    ht[0]='h';
    jstring sample=(*env)->NewStringUTF(env,ht);
    (*env)->CallVoidMethod(env, request_object, set_request_body_method,sample);
    (*env)->CallVoidMethod(env, handler_object, first_method,request_object,response_object);
    jstring s=(jstring)(*env)->CallObjectMethod(env, response_object, get_response_body_method,response_object);
    const char *str =(*env)->GetStringUTFChars(env,s,0);
    printf("output is %s\n",str);
    (*env)->ReleaseStringUTFChars(env,s, str);
}

int main(int argc, char **argv)
{
    JavaVM *jvm;
    JNIEnv *env;
    env = create_vm(&jvm);
    if(env == NULL)
        return 1;
    invoke_class(env);
    return 0;
}
