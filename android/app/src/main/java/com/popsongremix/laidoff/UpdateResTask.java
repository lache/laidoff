package com.popsongremix.laidoff;

import android.app.Activity;
import android.app.ProgressDialog;
import android.os.AsyncTask;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicLong;

class UpdateResTask extends AsyncTask<UpdateResTaskParam, Void, File> {

    private final ProgressDialog asyncDialog;
    private String fileAbsolutePath;
    private ArrayList<String> assetFile = new ArrayList<>();
    private GetFileResult listGfr;
    private final AtomicLong sequenceNumber = new AtomicLong(0);
    private String remoteBasePath;

    UpdateResTask(Activity activity) {
        asyncDialog = new ProgressDialog(activity);
    }

    @Override
    protected void onPreExecute() {
        asyncDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        asyncDialog.setMessage("로딩중입니다...");

        // show dialog
        asyncDialog.show();
        super.onPreExecute();
    }

    @Override
    protected File doInBackground(UpdateResTaskParam... params) {

        try {
            GetFileResult gfr = DownloadTask.getFile(
                    params[0].fileAbsolutePath,
                    params[0].remoteBasePath + "/" + params[0].remoteListFilePath,
                    params[0].localListFilename,
                    false
            );

            fileAbsolutePath = params[0].fileAbsolutePath;
            remoteBasePath = params[0].remoteBasePath;


            File listFile = new File(params[0].fileAbsolutePath, params[0].localListFilename);
            String[] assetFileList = DownloadTask.getStringFromFile(listFile).split("\n");
            for (String anAssetFileList : assetFileList) {
                String assetFilename = anAssetFileList.split("\t")[0];
                assetFile.add(assetFilename.replace("assets/", ""));
            }

            listGfr = gfr;

            return gfr.file;
        } catch (Exception e) {
            e.printStackTrace();
        }

        return null;
    }

    @Override
    protected void onPostExecute(File result) {
        // TODO: check this.exception
        // TODO: do something with the result

        updateAssetOneByOne();

        asyncDialog.dismiss();
        super.onPostExecute(result);
    }

    private void updateAssetOneByOne() {
        if (listGfr.newlyDownloaded) {
            for (int i = 0; i < assetFile.size(); i++) {

                String filename = assetFile.get(i);
                String filenameOnly = filename.substring(filename.lastIndexOf("/")+1);

                DownloadTaskParams dtp = new DownloadTaskParams();
                dtp.fileAbsolutePath = fileAbsolutePath;
                dtp.remotePath = remoteBasePath + "/" + filename;
                dtp.localFilename = filenameOnly;
                dtp.writeEtag = true;
                dtp.totalSequenceNumber = assetFile.size();
                dtp.sequenceNumber = sequenceNumber;
                dtp.urt = this;

                new DownloadTask().execute(dtp);
            }
        } else {
            Log.i(LaidOffNativeActivity.LOG_TAG, "Resource update not needed. (latest list file)");
            onResourceLoadFinished();
        }
    }

    void writeListEtag() {
        try {
            DownloadTask.writeEtagToFile(listGfr.etag, new File(listGfr.file.getAbsoluteFile() + ".Etag"));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    void onResourceLoadFinished() {
        String result = LaidOffNativeActivity.signalResourceReady(LaidOffNativeActivity.class);
        Log.i(LaidOffNativeActivity.LOG_TAG, "onResourceLoadFinished() [JAVA] - calling signalResourceReady() result: " + result);
    }
}
