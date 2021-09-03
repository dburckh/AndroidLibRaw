package com.homesoft.photo.androidlibraw

import androidx.appcompat.app.AppCompatActivity
import android.widget.ProgressBar
import android.widget.Toast
import android.content.Intent
import android.graphics.Bitmap
import android.system.Os
import android.system.OsConstants
import android.os.*
import android.util.Log
import android.view.View
import android.widget.ImageView
import androidx.annotation.StringRes
import com.homesoft.photo.libraw.LibRaw
import java.lang.Exception

class MainActivity : AppCompatActivity() {
    private lateinit var imageView: ImageView
    lateinit var progressBar: ProgressBar

    private val handler: Handler = object : Handler(Looper.getMainLooper()) {
        override fun handleMessage(msg: Message) {
            Toast.makeText(this@MainActivity, msg.what, Toast.LENGTH_LONG).show()
            progressBar.visibility = View.INVISIBLE
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        imageView = findViewById(R.id.imageView)
        progressBar = findViewById(R.id.progressBar)
        findViewById<View>(R.id.button).setOnClickListener { select() }
    }

    private fun select() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
        intent.addCategory(Intent.CATEGORY_OPENABLE)
        intent.type = "*/*"
        startActivityForResult(intent, PICK_FILE)
    }

    private fun showError(@StringRes error: Int) {
        handler.sendMessage(handler.obtainMessage(error))
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == PICK_FILE
            && resultCode == RESULT_OK
        ) {
            // The result data contains a URI for the document or directory that
            // the user selected.
            if (data != null) {
                imageView.setImageResource(R.drawable.ic_baseline_photo_camera_24)
                progressBar.visibility = View.VISIBLE
                data.data?.let { uri ->
                    // Perform operations on the document using its URI.
                    AsyncTask.SERIAL_EXECUTOR.execute {
                        try {
                            val pfd = contentResolver.openFileDescriptor(
                                uri, "r", null
                            )
                            if (pfd == null) {
                                showError(R.string.open_failed);
                                return@execute
                            }
                            //val bitmap = decodeMemory(pfd);
                            val bitmap = decodeFd(pfd);
                            if (bitmap == null) {
                                showError(R.string.decode_failed)
                            }
                            runOnUiThread {
                                progressBar.visibility = View.INVISIBLE
                                imageView.setImageBitmap(bitmap)
                            }
                        } catch (e: Exception) {
                            Log.e("Test", "Error", e)
                        }
                    }
                }
            }
        }
    }
    private fun decodeFd(pfd:ParcelFileDescriptor): Bitmap? {
        val fd = pfd.detachFd()
        val bitmap = LibRaw.decodeAsBitmap(fd, true)
        pfd.close()
        return bitmap
    }

    /**
     * Loads the file entirely into
     * May be faster
     */
    private fun decodeMemory(pfd:ParcelFileDescriptor): Bitmap? {
        val fd = pfd.fileDescriptor
        val structStat = Os.fstat(fd)
        val buffer = Os.mmap(
            0,
            structStat.st_size,
            OsConstants.PROT_READ,
            OsConstants.MAP_PRIVATE,
            fd,
            0
        )
        if (buffer < 0) {
            showError(R.string.map_failed)
            return null;
        }
        val bitmap = LibRaw.decodeAsBitmap(buffer, structStat.st_size.toInt(), true)
        Os.munmap(buffer, structStat.st_size)
        pfd.close()
        return bitmap;
    }

    companion object {
        const val PICK_FILE = 1234
    }
}