package me.pqpo.log4a;

import android.app.Application;

import me.pqpo.librarylog4a.Log4a;

/**
 * Created by pqpo on 2017/11/21.
 */
public class App extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        LogInit.init(this);
    }

    @Override
    public void onTerminate() {
        super.onTerminate();
        Log4a.release();
    }
}
