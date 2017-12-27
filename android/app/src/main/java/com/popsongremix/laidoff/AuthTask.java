package com.popsongremix.laidoff;

import android.os.AsyncTask;

import java.io.IOException;

import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class AuthTask extends AsyncTask<AuthTask.AuthTaskParam, Void, String> {
    public static class AuthTaskParam {
        public OkHttpClient client;
        String url;
        public String text;
    }
    public static final MediaType JSON
            = MediaType.parse("application/json; charset=utf-8");
    private static final MediaType TEXT_PLAIN
            = MediaType.parse("text/plain; charset=utf-8");
    @Override
    protected String doInBackground(AuthTaskParam... authTaskParams) {
        RequestBody body = RequestBody.create(TEXT_PLAIN, authTaskParams[0].text);
        Request request = new Request.Builder()
                .url(authTaskParams[0].url)
                .post(body)
                .build();
        Response response = null;
        try {
            response = authTaskParams[0].client.newCall(request).execute();
        } catch (IOException e) {
            e.printStackTrace();
        }
        try {
            if (response != null && response.body() != null) {
                return response.body().string();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "";
    }
}
