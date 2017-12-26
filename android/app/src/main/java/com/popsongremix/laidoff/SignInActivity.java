package com.popsongremix.laidoff;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.util.Log;

import com.google.android.gms.auth.api.Auth;
import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInClient;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.auth.api.signin.GoogleSignInResult;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.CommonStatusCodes;
import com.google.android.gms.games.Games;
import com.google.android.gms.games.Player;
import com.google.android.gms.games.PlayersClient;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.auth.AuthCredential;
import com.google.firebase.auth.AuthResult;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.GoogleAuthProvider;

public class SignInActivity extends Activity {
    int RC_SIGN_IN_2 = 20000;
    private FirebaseAuth mAuth;
    private GoogleSignInOptions gso;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mAuth = FirebaseAuth.getInstance();

        if (isSignedIn()) {
            signOut();
        } else {
            gso = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN)
                    // requestIdToken 인자는 "웹 애플리케이션 유형"의 클라이언트 ID임
                    .requestIdToken("933754889415-fr8buam0m8dh9hv71r2fb4au548lij0f.apps.googleusercontent.com")
                    .requestEmail()
                    .requestProfile()
                    .build();
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
                            GoogleSignInAccount signedInAccount = task.getResult();
                            //Log.i(LaidoffNativeActivity.LOG_TAG, signedInAccount.getDisplayName());
                            onSignedInAccountAcquired(signedInAccount);
                        } else {
                            // Player will need to sign-in explicitly using via UI
                            Log.i(LaidoffNativeActivity.LOG_TAG, "Failed!");
                            ApiException e = (ApiException)task.getException();
                            if (e != null) {
                                if (e.getStatusCode() == CommonStatusCodes.SIGN_IN_REQUIRED) {
                                    startSignInIntent2();
                                }
                            } else {
                                Log.e(LaidoffNativeActivity.LOG_TAG, "ApiException error");
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
                        Log.i(LaidoffNativeActivity.LOG_TAG, "Signed out successfully");
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

    @Override
    protected void onResume() {
        super.onResume();
        //signInSilently(gso);
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
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == RC_SIGN_IN_2) {
            GoogleSignInResult result = Auth.GoogleSignInApi.getSignInResultFromIntent(data);
            if (result.isSuccess()) {
                // The signed in account is stored in the result.
                GoogleSignInAccount signedInAccount = result.getSignInAccount();
                onSignedInAccountAcquired(signedInAccount);
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

    private void onSignedInAccountAcquired(GoogleSignInAccount signedInAccount) {
        Log.i(LaidoffNativeActivity.LOG_TAG, signedInAccount.getDisplayName());

        AuthCredential credential = GoogleAuthProvider.getCredential(signedInAccount.getIdToken(),null);

        mAuth.signInWithCredential(credential)
                .addOnCompleteListener(
                        new OnCompleteListener<AuthResult>() {
                            @Override
                            public void onComplete(@NonNull Task<AuthResult> task) {
                                // check task.isSuccessful()
                                if (task.isSuccessful()) {
                                    Log.i(LaidoffNativeActivity.LOG_TAG, "Registered to firebase successfully.");
                                } else {
                                    Log.e(LaidoffNativeActivity.LOG_TAG, "Registered to firebase failed!!!");
                                }
                            }
                        });

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
    }

    private void printGoogleAccountDebugInfo(GoogleSignInAccount account) {
        Log.i(LaidoffNativeActivity.LOG_TAG, "handleSignInResult - Google account name: " + account.getDisplayName());
        Log.i(LaidoffNativeActivity.LOG_TAG, "handleSignInResult - Google Photo URL: " + account.getPhotoUrl());
    }
}
