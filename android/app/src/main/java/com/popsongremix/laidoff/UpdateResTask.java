package com.popsongremix.laidoff;

import android.graphics.Path;
import android.os.AsyncTask;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

public class UpdateResTask extends AsyncTask<String, Void, File> {

    String fileAbsolutePath;
    ArrayList<String> assetFile = new ArrayList<String>();

    protected File doInBackground(String... params) {

        try {
            File file = DownloadTask.getFile(params[0], params[1], params[2]);

            fileAbsolutePath = params[0];

            File listFile = new File(params[0], params[2]);
            String[] assetFileList = DownloadTask.getStringFromFile(listFile).split("\n");
            for (int i = 0; i < assetFileList.length; i++) {
                String assetFilename = assetFileList[i].split("\t")[0];
                assetFile.add(assetFilename.replace("assets/", ""));
            }
            return file;
        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }

        return null;
    }

    protected void onPostExecute(File feed) {
        // TODO: check this.exception
        // TODO: do something with the feed

        for (int i = 0; i < assetFile.size(); i++) {

            String filename = assetFile.get(i);
            String filenameOnly = filename.substring(filename.lastIndexOf("/")+1);

            new DownloadTask().execute(
                    fileAbsolutePath,
                    "http://222.110.4.119:18080/" + filename,
                    filenameOnly);
        }
    }
}