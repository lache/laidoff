package com.popsongremix.laidoff;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicLong;

class UpdateResTask extends AsyncTask<UpdateResTaskParam, Void, File> {

    private final ProgressDialog asyncDialog;
    private final Activity activity;
    private String fileAbsolutePath;
    private ArrayList<String> assetFile = new ArrayList<>();
    private GetFileResult listGfr;
    private final AtomicLong sequenceNumber = new AtomicLong(0);
    private String remoteBasePath;

    UpdateResTask(Activity activity) {
        asyncDialog = new ProgressDialog(activity);
        this.activity = activity;
    }

    @Override
    protected void onPreExecute() {
        asyncDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        asyncDialog.setMessage("로딩중");
        asyncDialog.setMax(1);

        // show dialog
        asyncDialog.show();
        super.onPreExecute();
    }

    @Override
    protected File doInBackground(UpdateResTaskParam... params) {

        try {
            checkNewVersion(params[0]);

            getListFile(params[0]);

            asyncDialog.setProgress(1);

            updateAssetOneByOneSync();

            return listGfr.file;
        } catch (UnknownHostException e) {
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    AlertDialog.Builder dlgAlert  = new AlertDialog.Builder(activity);
                    dlgAlert.setMessage(R.string.no_conn);
                    dlgAlert.setTitle(R.string.error);
                    dlgAlert.setPositiveButton(R.string.exit, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            activity.finish();
                        }
                    });
                    //dlgAlert.setCancelable(true);
                    dlgAlert.create().show();
                    //Toast.makeText(activity.getApplicationContext(), R.string.no_conn, Toast.LENGTH_LONG).show();
                }
            });
        } catch (Exception e) {
            e.printStackTrace();
        }

        return null;
    }

    private void checkNewVersion(UpdateResTaskParam param) throws Exception {
        String packageVersionName = "";
        try {
            PackageInfo packageInfo = activity.getPackageManager().getPackageInfo(activity.getPackageName(), 0);
            packageVersionName = packageInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }

        if (packageVersionName.compareTo("0.1.0") == 0) {
            Log.i(LaidOffNativeActivity.LOG_TAG, "Development version name detected. Version check will be skipped.");
        } else {
            GetFileResult versionNameFileResult = DownloadTask.getFile(
                    param.fileAbsolutePath,
                    param.remoteApkBasePath + "/versionName.txt",
                    "versionName.txt",
                    false
            );

            String versionNameFromServer = DownloadTask.getStringFromFile(versionNameFileResult.file).trim();
            if (packageVersionName.compareTo(versionNameFromServer) != 0) {
                activity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(activity.getApplicationContext(), "최신 버전이 있습니다.", Toast.LENGTH_SHORT).show();
                    }
                });
                Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(param.remoteApkBasePath + "/laidoff.apk"));
                activity.startActivity(browserIntent);
            }
        }
    }

    private void getListFile(UpdateResTaskParam param) throws Exception {
        GetFileResult gfr = DownloadTask.getFile(
                param.fileAbsolutePath,
                param.remoteAssetsBasePath + "/" + param.remoteListFilePath,
                param.localListFilename,
                false
        );

        fileAbsolutePath = param.fileAbsolutePath;
        remoteBasePath = param.remoteAssetsBasePath;


        File listFile = new File(param.fileAbsolutePath, param.localListFilename);
        String[] assetFileList = DownloadTask.getStringFromFile(listFile).split("\n");
        for (String anAssetFileList : assetFileList) {
            String assetFilename = anAssetFileList.split("\t")[0];
            assetFile.add(assetFilename.replace("assets/", ""));
        }

        listGfr = gfr;
    }

    @Override
    protected void onPostExecute(File result) {
        // TODO: check this.exception
        // TODO: do something with the result

        //updateAssetOneByOneUsingTask();

        asyncDialog.dismiss();
        super.onPostExecute(result);
    }

    @SuppressWarnings("unused")
    private void updateAssetOneByOneUsingTask() {
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

    private void updateAssetOneByOneSync() {
        if (listGfr.newlyDownloaded) {
            asyncDialog.setMax(assetFile.size());
            for (int i = 0; i < assetFile.size(); i++) {

                String filename = assetFile.get(i);
                final String filenameOnly = filename.substring(filename.lastIndexOf("/")+1);

                /*
                activity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        asyncDialog.setMessage("다운로딩: " + filenameOnly);
                    }
                });
                */

                DownloadTaskParams dtp = new DownloadTaskParams();
                dtp.fileAbsolutePath = fileAbsolutePath;
                dtp.remotePath = remoteBasePath + "/" + filename;
                dtp.localFilename = filenameOnly;
                dtp.writeEtag = true;
                dtp.totalSequenceNumber = assetFile.size();
                dtp.sequenceNumber = sequenceNumber;
                dtp.urt = this;

                try {
                    DownloadTask.getFile(dtp.fileAbsolutePath, dtp.remotePath, dtp.localFilename, dtp.writeEtag);
                } catch (Exception e) {
                    e.printStackTrace();
                }

                asyncDialog.setProgress(i + 1);

                if (dtp.sequenceNumber.incrementAndGet() == dtp.totalSequenceNumber) {
                    Log.i(LaidOffNativeActivity.LOG_TAG, "Resource update finished.");
                    dtp.urt.writeListEtag();
                    dtp.urt.onResourceLoadFinished();
                }
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
