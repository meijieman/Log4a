package me.pqpo.log4a;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import me.pqpo.librarylog4a.Level;
import me.pqpo.librarylog4a.Log4a;
import me.pqpo.librarylog4a.LogData;
import me.pqpo.librarylog4a.appender.AndroidAppender;
import me.pqpo.librarylog4a.appender.FileAppender;
import me.pqpo.librarylog4a.formatter.DateFileFormatter;
import me.pqpo.librarylog4a.interceptor.Interceptor;
import me.pqpo.librarylog4a.logger.AppenderLogger;

/**
 * Created by pqpo on 2017/11/24.
 */
public class LogInit {

    public static final int BUFFER_SIZE = 1024 * 2; //400k

    public static void init(Context context) {
        int level = Level.DEBUG;
        Interceptor wrapInterceptor = new Interceptor() {
            @Override
            public boolean intercept(LogData logData) {
                logData.tag = "Log4a-" + logData.tag;
                return true;
            }
        };
        AndroidAppender androidAppender = new AndroidAppender.Builder()
                .setLevel(level)
                .addInterceptor(wrapInterceptor)
                .create();

//        File log = FileUtils.getLogDir(context);
        File log = new File(Environment.getExternalStorageDirectory() + File.separator + "log");
        log.mkdirs();
        String buffer_path = log.getAbsolutePath() + File.separator + ".logCache";
        String time = new SimpleDateFormat("yyyy_MM_dd", Locale.getDefault()).format(new Date());
        String log_path = log.getAbsolutePath() + File.separator + time + ".txt";
        Log.w(MainActivity.TAG, "log_path " + log_path);
        FileAppender fileAppender = new FileAppender.Builder(context)
                .setLogFilePath(log_path)
                .setFileMaxSize(1024 * 2) // 2kb
                .setLevel(level)
                .addInterceptor(wrapInterceptor)
                .setBufferFilePath(buffer_path)
                .setFormatter(new DateFileFormatter())
                .setCompress(false)
                .setBufferSize(BUFFER_SIZE)
                .create();

        AppenderLogger logger = new AppenderLogger.Builder()
                .addAppender(androidAppender)
                .addAppender(fileAppender)
                .create();
        Log4a.setLogger(logger);
    }

}
