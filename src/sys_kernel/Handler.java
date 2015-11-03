public class Handler{
	public static int first(Request req,Response res){
		res.setResponseBody(req.getRequestBody()+"|");
		return 1;
	}
}
