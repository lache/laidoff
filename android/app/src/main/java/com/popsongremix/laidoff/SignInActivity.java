package com.popsongremix.laidoff;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.util.Log;
import android.widget.Toast;

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
    int RC_SIGN_IN = 20000;
    private GoogleSignInOptions googleSignInOptions;
    public static final String LOG_TAG = "SignIn";
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (isSignedIn() == false) {
            googleSignInOptions = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN)
                    .requestIdToken(getString(R.string.web_client_id))
                    .requestEmail()
                    .requestProfile()
                    .build();
            signInSilently(googleSignInOptions);
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
                            //Log.i(LOG_TAG, signedInAccount.getDisplayName());
                            onSignedInAccountAcquired(signedInAccount);
                        } else {
                            // Player will need to sign-in explicitly using via UI
                            ApiException e = (ApiException)task.getException();
                            if (e != null) {
                                if (e.getStatusCode() == CommonStatusCodes.SIGN_IN_REQUIRED) {
                                    Log.i(LOG_TAG, "Silent sign-in failed. Try to start user interaction sign-in activity...");
                                    startSignInIntent();
                                } else {
                                    Log.e(LOG_TAG, String.format("Silent sign-in failed with unknown status code %d", e.getStatusCode()));
                                }
                            } else {
                                Log.e(LOG_TAG, "Silent sign-in failed. ApiException null error.");
                            }
                        }
                    }
                });
    }

    private void startSignInIntent() {
        GoogleSignInClient signInClient = GoogleSignIn.getClient(this, googleSignInOptions);
        Intent intent = signInClient.getSignInIntent();
        startActivityForResult(intent, RC_SIGN_IN);
    }

    private boolean isSignedIn() {
        return GoogleSignIn.getLastSignedInAccount(this) != null;
    }

    @Override
    protected void onResume() {
        super.onResume();
        //signInSilently(googleSignInOptions);
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
            Log.i(LOG_TAG, "onStart(): Google account not available.");
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == RC_SIGN_IN) {
            GoogleSignInResult result = Auth.GoogleSignInApi.getSignInResultFromIntent(data);
            if (result.isSuccess()) {
                // The signed in account is stored in the result.
                GoogleSignInAccount signedInAccount = result.getSignInAccount();
                onSignedInAccountAcquired(signedInAccount);
            } else {
                String message = result.getStatus().getStatusMessage();
                if (message == null || message.isEmpty()) {
                    message = "Unknown sign-in error!";
                }
                new AlertDialog.Builder(this).setMessage(message)
                        .setNeutralButton(android.R.string.ok, null).show();
            }
        }
    }

    private void onSignedInAccountAcquired(final GoogleSignInAccount signedInAccount) {
        Log.i(LOG_TAG, "Account Display Name: " + signedInAccount.getDisplayName());
        Log.i(LOG_TAG, "Account Photo URL: " + signedInAccount.getPhotoUrl());
        Log.i(LOG_TAG, "Account Email: " + signedInAccount.getEmail());
        Log.i(LOG_TAG, "Account Family Name: " + signedInAccount.getFamilyName());
        Log.i(LOG_TAG, "Account Given Name: " + signedInAccount.getGivenName());
        Log.i(LOG_TAG, "Account ID Token: " + signedInAccount.getIdToken());
        Log.i(LOG_TAG, "Account ID: " + signedInAccount.getId());
        AuthCredential credential = GoogleAuthProvider.getCredential(signedInAccount.getIdToken(),null);
        final Activity activity = this;
        // Signed-in successfully. Register this account to firebase.
        FirebaseAuth.getInstance().signInWithCredential(credential)
                .addOnCompleteListener(
                        new OnCompleteListener<AuthResult>() {
                            @Override
                            public void onComplete(@NonNull Task<AuthResult> task) {
                                // check task.isSuccessful()
                                if (task.isSuccessful()) {
                                    Log.i(LOG_TAG, "Registered to firebase successfully.");
                                    Toast.makeText(activity, "Registered to firebase successfully.", Toast.LENGTH_LONG).show();
                                } else {
                                    Log.e(LOG_TAG, "Registered to firebase failed with error: " + task.toString());
                                }
                                // Get Google Play Games profile info
                                PlayersClient pc = Games.getPlayersClient(activity, signedInAccount);
                                pc.getCurrentPlayer().addOnCompleteListener(activity,
                                        new OnCompleteListener<Player>() {
                                            @Override
                                            public void onComplete(@NonNull Task<Player> task) {
                                                if (task.isSuccessful()) {
                                                    Player player = task.getResult();
                                                    Log.i(LOG_TAG, "Player Name: " + player.getName());
                                                    Log.i(LOG_TAG, "Player Display Name: " + player.getDisplayName());
                                                    Log.i(LOG_TAG, "Player ID: " + player.getPlayerId());
                                                    Log.i(LOG_TAG, "Player Title: " + player.getTitle());
                                                    Log.i(LOG_TAG, "Player Hi Res Image URI: " + player.getHiResImageUri());
                                                    Log.i(LOG_TAG, "Player Icon Image URI: " + player.getIconImageUri());
                                                    Log.i(LOG_TAG, "Player Banner Image Landscape URI: " + player.getBannerImageLandscapeUri());
                                                    Log.i(LOG_TAG, "Player Banner Image Portrait URI: " + player.getBannerImagePortraitUri());
                                                    Log.i(LOG_TAG, "Player Has Icon Image: " + player.hasIconImage());
                                                } else {
                                                    Log.e(LOG_TAG, "getCurrentPlayer() error: " + task.toString());
                                                }
                                                activity.finish();
                                            }
                                        });
                            }
                        });
    }

    private void printGoogleAccountDebugInfo(GoogleSignInAccount account) {
        Log.i(LOG_TAG, "handleSignInResult - Google account name: " + account.getDisplayName());
        Log.i(LOG_TAG, "handleSignInResult - Google Photo URL: " + account.getPhotoUrl());
    }
}
