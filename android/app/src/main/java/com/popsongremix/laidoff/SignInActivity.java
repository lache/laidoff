package com.popsongremix.laidoff;

import android.app.AlertDialog;
import android.content.Intent;
import android.content.IntentSender;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.os.Bundle;
import android.util.Log;

import com.google.android.gms.auth.api.Auth;
import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.auth.api.signin.GoogleSignInOptionsExtension;
import com.google.android.gms.auth.api.signin.GoogleSignInResult;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.CommonStatusCodes;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.ResultCallback;
import com.google.android.gms.games.Games;
import com.google.android.gms.games.Player;
import com.google.android.gms.games.PlayersClient;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.auth.AuthCredential;
import com.google.firebase.auth.AuthResult;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.GoogleAuthProvider;

public class SignInActivity extends FragmentActivity implements GoogleApiClient.OnConnectionFailedListener, GoogleApiClient.ConnectionCallbacks {
    private GoogleSignInClient mGoogleSignInClient;
    private GoogleApiClient mGoogleApiClient;
    int RC_SIGN_IN = 10000;
    int RC_SIGN_IN_2 = 20000;
    int RC_RESOLVE_ERR = 30000;
    private FirebaseAuth mAuth;
    private GoogleSignInOptions gso;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (isSignedIn()) {
            signOut();
        }

        signinxxxxxxx();

    }

    private void signinxxxxxxx() {
        if (isSignedIn() == false) {
            // Configure sign-in to request the user's ID, email address, and basic
            // profile. ID and basic profile are included in DEFAULT_SIGN_IN.
            gso = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN)
                    // requestIdToken 인자는 "웹 애플리케이션 유형"의 클라이언트 ID임
                    //.requestIdToken("933754889415-fr8buam0m8dh9hv71r2fb4au548lij0f.apps.googleusercontent.com")
                    .requestProfile()
                    .build();
        mGoogleApiClient = new GoogleApiClient.Builder(this)
                .enableAutoManage(this, this)
                .addConnectionCallbacks(this)
                //.addApi(Games.API)
                //.addScope(Games.SCOPE_GAMES)
                .addApi(Auth.GOOGLE_SIGN_IN_API, gso)
                .build();
            // Build a GoogleSignInClient with the options specified by gso.

            mAuth = FirebaseAuth.getInstance();

            //Intent signInIntent = Auth.GoogleSignInApi.getSignInIntent(mGoogleApiClient);
            //startActivityForResult(signInIntent, RC_SIGN_IN);

            //mGoogleSignInClient = GoogleSignIn.getClient(this, gso);
            //signIn();

            signInSilently(gso);
        }
    }

    private void signInSilently(GoogleSignInOptions gso) {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(this, gso);
        signInClient.silentSignIn().addOnCompleteListener(this,
                new OnCompleteListener<GoogleSignInAccount>() {
                    @Override
                    public void onComplete(@NonNull Task<GoogleSignInAccount> task) {
                        if (task.isSuccessful()) {
                            // The signed in account is stored in the task's result.
                            try {
                                GoogleSignInAccount signedInAccount = task.getResult(ApiException.class);
                                Log.i(LaidoffNativeActivity.LOG_TAG, signedInAccount.getDisplayName());
                            } catch (ApiException e) {
                                if (e.getStatusCode() == CommonStatusCodes.SIGN_IN_REQUIRED) {
                                    startSignInIntent2();
                                }
                            }

                        } else {
                            // Player will need to sign-in explicitly using via UI
                            Log.i(LaidoffNativeActivity.LOG_TAG, "Failed!");
                            ApiException e = (ApiException)task.getException();
                            if (e.getStatusCode() == CommonStatusCodes.SIGN_IN_REQUIRED) {
                                startSignInIntent2();
                            }
                        }
                    }
                });
    }

    private void signOut() {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(this,
                GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN);
        signInClient.signOut().addOnCompleteListener(this,
                new OnCompleteListener<Void>() {
                    @Override
                    public void onComplete(@NonNull Task<Void> task) {
                        // at this point, the user is signed out.
                    }
                });
    }

    private void startSignInIntent2() {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(this, gso);
        Intent intent = signInClient.getSignInIntent();
        startActivityForResult(intent, RC_SIGN_IN_2);
    }

    private boolean isSignedIn() {
        return GoogleSignIn.getLastSignedInAccount(this) != null;
    }

    private void startSignInIntent() {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(this,
                GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN);
        Intent intent = signInClient.getSignInIntent();
        startActivityForResult(intent, RC_SIGN_IN);
    }

    @Override
    protected void onResume() {
        super.onResume();
        //signInSilently();
    }

    @Override
    public void onStart() {
        super.onStart();
        // Check for existing Google Sign In account, if the user is already signed in
        // the GoogleSignInAccount will be non-null.
        GoogleSignInAccount account = GoogleSignIn.getLastSignedInAccount(this);
        if (account != null) {
            printGoogleAccountDebugInfo(account);
        } else {
            Log.i(LaidoffNativeActivity.LOG_TAG, "onStart(): Google account not available.");
        }
    }

    @Override
    public void onConnectionFailed(@NonNull ConnectionResult connectionResult) {
        if (connectionResult.hasResolution()) {
            try {
                connectionResult.startResolutionForResult(this, RC_RESOLVE_ERR);
            } catch (IntentSender.SendIntentException e) {

            }
        }

    }

    @Override
    public void onConnected(@Nullable Bundle bundle) {

            Auth.GoogleSignInApi.silentSignIn(mGoogleApiClient).setResultCallback(
                    new ResultCallback<GoogleSignInResult>() {
                        @Override
                        public void onResult(GoogleSignInResult googleSignInResult) {
                            GoogleSignInAccount acct = googleSignInResult.getSignInAccount();
                            if (acct == null) {
                                Log.e(LaidoffNativeActivity.LOG_TAG, "account was null: " + googleSignInResult.getStatus().getStatusMessage());
                                return;
                            }

                            AuthCredential credential = GoogleAuthProvider.getCredential(acct.getIdToken(),null);

                            mAuth.signInWithCredential(credential)
                                    .addOnCompleteListener(
                                            new OnCompleteListener<AuthResult>() {
                                                @Override
                                                public void onComplete(@NonNull Task<AuthResult> task) {
                                                    // check task.isSuccessful()
                                                }
                                            });
                        }
                    }
            );

    }

    @Override
    public void onConnectionSuspended(int i) {

    }

    public void signIn() {
        Intent signInIntent = mGoogleSignInClient.getSignInIntent();
        startActivityForResult(signInIntent, RC_SIGN_IN);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        // Result returned from launching the Intent from GoogleSignInClient.getSignInIntent(...);
        if (requestCode == RC_SIGN_IN) {
            // The Task returned from this call is always completed, no need to attach
            // a listener.
            Task<GoogleSignInAccount> task = GoogleSignIn.getSignedInAccountFromIntent(data);
            handleSignInResult(task);
        }

        if (requestCode == RC_SIGN_IN_2) {
            GoogleSignInResult result = Auth.GoogleSignInApi.getSignInResultFromIntent(data);
            if (result.isSuccess()) {
                // The signed in account is stored in the result.
                GoogleSignInAccount signedInAccount = result.getSignInAccount();
                Log.i(LaidoffNativeActivity.LOG_TAG, signedInAccount.getDisplayName());

                PlayersClient pc = Games.getPlayersClient(this, signedInAccount);
                pc.getCurrentPlayer().addOnCompleteListener(this,
                        new OnCompleteListener<Player>() {
                            @Override
                            public void onComplete(@NonNull Task<Player> task) {
                                if (task.isSuccessful()) {
                                    Player player = task.getResult();
                                    Log.i(LaidoffNativeActivity.LOG_TAG, player.getName());
                                } else {
                                    Log.e(LaidoffNativeActivity.LOG_TAG, "ERROR~");
                                }
                            }
                        });
            } else {

                String message = result.getStatus().getStatusMessage();
                if (message == null || message.isEmpty()) {
                    message = "sign in error!!";
                }
                new AlertDialog.Builder(this).setMessage(message)
                        .setNeutralButton(android.R.string.ok, null).show();
            }
        }
    }

    private void handleSignInResult(Task<GoogleSignInAccount> completedTask) {
        try {
            GoogleSignInAccount account = completedTask.getResult(ApiException.class);
            printGoogleAccountDebugInfo(account);
            // Signed in successfully, show authenticated UI.
            //updateUI(account);
        } catch (ApiException e) {
            // The ApiException status code indicates the detailed failure reason.
            // Please refer to the GoogleSignInStatusCodes class reference for more information.
            Log.w(LaidoffNativeActivity.LOG_TAG, "handleSignInResult - signInResult:failed code=" + e.getStatusCode());
            //updateUI(null);
        }
    }

    private void printGoogleAccountDebugInfo(GoogleSignInAccount account) {
        Log.i(LaidoffNativeActivity.LOG_TAG, "handleSignInResult - Google account name: " + account.getDisplayName());
        Log.i(LaidoffNativeActivity.LOG_TAG, "handleSignInResult - Google Photo URL: " + account.getPhotoUrl());
    }
}
