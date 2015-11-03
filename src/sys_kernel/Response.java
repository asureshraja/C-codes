import java.util.*;
public class Response{
    private static String responseBody;
    private HashMap<String,String> headers=new HashMap<String,String>();
    public void setResponseBody(String responsBody){
        responseBody=responsBody;
    }
    public static String getResponseBody(){
        return responseBody;
    }

    public void addHeader(String key,String value){
        headers.put(key,value);
    }

    public String getHeaderValue(String key){
        return headers.get(key);
    }
}
