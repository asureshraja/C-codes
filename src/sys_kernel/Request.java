import java.util.*;
public class Request{
    private static String body;
    private HashMap<String,String> headers=new HashMap<String,String>();
    public static int setRequestBody(String bod){
        body=bod;
        return 1;
    }
    public String getRequestBody(){
        return this.body;
    }

    public void addHeader(String key,String value){
        headers.put(key,value);
    }

    public String getHeaderValue(String key){
        return headers.get(key);
    }
}
