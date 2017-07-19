package ca.raido.glSatelliteDemo;

import android.app.Application;
import android.os.Environment;
import android.os.StrictMode;
import android.util.Log;

import com.squareup.leakcanary.AndroidExcludedRefs;
import com.squareup.leakcanary.AndroidHeapDumper;
import com.squareup.leakcanary.AndroidRefWatcherBuilder;
import com.squareup.leakcanary.DisplayLeakService;
import com.squareup.leakcanary.HeapDumper;
import com.squareup.leakcanary.LeakCanary;
import com.squareup.leakcanary.LeakDirectoryProvider;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.UUID;

public class GlobeApplication extends Application {
    public static final boolean DEVELOPER_MODE = false;
    private static final boolean STRICT_MODE = false;

    @Override
    public void onCreate() {
        super.onCreate();
        if (DEVELOPER_MODE) {
            Log.d(getApplicationContext().getPackageName(), "onCreate called");
            if (LeakCanary.isInAnalyzerProcess(this)) {
                // This process is dedicated to LeakCanary for heap analysis.
                // You should not init your app in this process.
                return;
            }
            final LeakDirectoryProvider leakDirectoryProvider = new CacheDirectoryProvider();
            final AndroidRefWatcherBuilder builder = LeakCanary.refWatcher(this);
            builder.heapDumper(new AndroidHeapDumper(this, leakDirectoryProvider));
            LeakCanary.setDisplayLeakActivityDirectoryProvider(leakDirectoryProvider);
            builder.listenerServiceClass(DisplayLeakService.class)
                .excludedRefs(AndroidExcludedRefs.createAppDefaults().build())
                .buildAndInstall();

            if (STRICT_MODE) {
                StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                    .detectAll().penaltyLog().build());
                StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                    .detectAll().penaltyLog().penaltyDeath().build());
            }
        }
    }

    private final class CacheDirectoryProvider implements LeakDirectoryProvider {
        private static final String TAG = "LeakCanary";
        private static final String HPROF_SUFFIX = ".hprof";
        private static final String PENDING_HEAPDUMP_SUFFIX = "_pending" + HPROF_SUFFIX;
        /* 10 minutes. */
        private static final int ANALYSIS_MAX_DURATION_MS = 10 * 60 * 1000;
        private static final int DEFAULT_MAX_STORED_HEAP_DUMPS = 7;

        @Override
        public List<File> listFiles(FilenameFilter filter) {
            final List<File> files = new ArrayList<>();

            final File[] cacheFiles = cacheDirectory().listFiles(filter);
            if (cacheFiles != null) {
                files.addAll(Arrays.asList(cacheFiles));
            }

            return files;
        }

        @Override
        public File newHeapDumpFile() {
            final List<File> pendingHeapDumps = listFiles(new FilenameFilter() {
                @Override public boolean accept(File dir, String filename) {
                    return filename.endsWith(PENDING_HEAPDUMP_SUFFIX);
                }
            });

            // If a new heap dump file has been created recently and hasn't been processed yet, we skip.
            // Otherwise we move forward and assume that the analyzer process crashes. The file will
            // eventually be removed with heap dump file rotation.
            for (File file : pendingHeapDumps) {
                if (System.currentTimeMillis() - file.lastModified() < ANALYSIS_MAX_DURATION_MS) {
                    Log.d(TAG, "Could not dump heap, previous analysis still is in progress.");
                    return HeapDumper.RETRY_LATER;
                }
            }

            cleanupOldHeapDumps();

            final File storageDirectory = cacheDirectory();
            if (!directoryWritableAfterMkdirs(storageDirectory)) {
                final String state = Environment.getExternalStorageState();
                if (!Environment.MEDIA_MOUNTED.equals(state)) {
                    Log.d(TAG, String.format("External storage not mounted, state: %s", state));
                } else {
                    Log.d(TAG, String.format("Could not create heap dump directory in external storage: [%s]",
                        storageDirectory.getAbsolutePath()));
                }
            }
            // If two processes from the same app get to this step at the same time, they could both
            // create a heap dump. This is an edge case we ignore.
            return new File(storageDirectory, UUID.randomUUID().toString() + PENDING_HEAPDUMP_SUFFIX);
        }

        @Override
        public void clearLeakDirectory() {
            final List<File> allFilesExceptPending = listFiles(new FilenameFilter() {
                @Override public boolean accept(File dir, String filename) {
                    return !filename.endsWith(PENDING_HEAPDUMP_SUFFIX);
                }
            });
            for (File file : allFilesExceptPending) {
                final boolean deleted = file.delete();
                if (!deleted) {
                    Log.d(TAG, String.format("Could not delete file %s", file.getPath()));
                }
            }
        }

        private boolean directoryWritableAfterMkdirs(File directory) {
            final boolean success = directory.mkdirs();
            return (success || directory.exists()) && directory.canWrite();
        }

        private File cacheDirectory() {
            final File cacheDir = getExternalCacheDir();
            return new File(cacheDir, "leakcanary-" + getPackageName());
        }

        private void cleanupOldHeapDumps() {
            final List<File> hprofFiles = listFiles(new FilenameFilter() {
                @Override public boolean accept(File dir, String filename) {
                    return filename.endsWith(HPROF_SUFFIX);
                }
            });
            final int filesToRemove = hprofFiles.size() - DEFAULT_MAX_STORED_HEAP_DUMPS;
            if (filesToRemove > 0) {
                Log.d(TAG, String.format("Removing %d heap dumps", filesToRemove));
                // Sort with oldest modified first.
                Collections.sort(hprofFiles, new Comparator<File>() {
                    @Override public int compare(File lhs, File rhs) {
                        return Long.valueOf(lhs.lastModified()).compareTo(rhs.lastModified());
                    }
                });
                for (int i = 0; i < filesToRemove; i++) {
                    final File file = hprofFiles.get(i);
                    final boolean deleted = file.delete();
                    if (!deleted) {
                        Log.d(TAG, String.format("Could not delete old hprof file %s", file.getPath()));
                    }
                }
            }
        }
    }
}
