package com.homesoft.photo.androidlibraw

import androidx.appcompat.app.AppCompatActivity
import android.widget.ProgressBar
import android.widget.Toast
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.system.Os
import android.system.OsConstants
import android.os.*
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.ImageView
import androidx.annotation.StringRes
import com.homesoft.photo.libraw.LibRaw
import java.lang.Exception
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() {
    private val executor = Executors.newSingleThreadExecutor()
    private lateinit var raw: ImageView
    private lateinit var jpeg: ImageView
    private lateinit var button: Button
    lateinit var progressBar: ProgressBar

    private val handler: Handler = object : Handler(Looper.getMainLooper()) {
        override fun handleMessage(msg: Message) {
            Toast.makeText(this@MainActivity, msg.what, Toast.LENGTH_LONG).show()
            setBusy(false)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        raw = findViewById(R.id.raw)
        jpeg = findViewById(R.id.jpeg)
        progressBar = findViewById(R.id.progressBar)
        button = findViewById(R.id.button)
        button.setOnClickListener { select() }
    }

    override fun onDestroy() {
        super.onDestroy()
        executor.shutdownNow()
    }

    private fun setBusy(busy:Boolean) {
        if (busy) {
            button.visibility = View.INVISIBLE
            progressBar.visibility = View.VISIBLE
        } else {
            button.visibility = View.VISIBLE
            progressBar.visibility = View.INVISIBLE
        }
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
                raw.setImageResource(R.drawable.ic_baseline_photo_camera_24)
                jpeg.setImageResource(R.drawable.ic_baseline_photo_camera_24)
                setBusy(true)
                data.data?.let { uri ->
                    // Perform operations on the document using its URI.
                    executor.execute {
                        try {
                            val pfd = contentResolver.openFileDescriptor(
                                uri, "r", null
                            )
                            if (pfd == null) {
                                showError(R.string.open_failed);
                                return@execute
                            }
                            //val bitmap = decodeMemory(pfd)
                            val jpegBitmap = decodeEmbeddedJpeg(pfd)
                            val rawBitmap = decodeFd(pfd)
                            pfd.close()
//                            if (raw == null || jpeg == null) {
//                                showError(R.string.decode_failed)
//                                return@execute
//                            }
                            runOnUiThread {
                                setBusy(false)
                                raw.setImageBitmap(rawBitmap)
                                jpeg.setImageBitmap(jpegBitmap)
                            }
//                            val list = decodeTiles(pfd);
//                            handler.post { progressBar.visibility = View.INVISIBLE }
//                            for (i in list.indices) {
//                                handler.postDelayed ({
//                                    raw.setImageBitmap(list[i])
//                                }, i * 250L)
//                            }
                        } catch (e: Exception) {
                            Log.e("Test", "Error", e)
                        }
                    }
                }
            }
        }
    }

    /**
     * Decoded using the JPEG embedded in the RAW
     */
    private fun decodeEmbeddedJpeg(pfd:ParcelFileDescriptor):Bitmap? {
        val options = BitmapFactory.Options()
        options.inJustDecodeBounds
        BitmapFactory.decodeFileDescriptor(pfd.fileDescriptor, null, options)
        Log.d("Test", "Size ${options.outWidth}x${options.outHeight}")
        if (options.outWidth * options.outHeight > 12_000_000) {
            options.inSampleSize = 2
        }
        val bitmap = BitmapFactory.decodeFileDescriptor(pfd.fileDescriptor, null, options)
        return bitmap
    }


    private fun decodeTiles(pfd:ParcelFileDescriptor):List<Bitmap> {
        LibRaw.setQuality(2)
        val result = unpackFd(pfd)
        val list = mutableListOf<Bitmap>()
        val size = 256
        if (result == 0) {
            for (i in 0..8) {
                LibRaw.setCropBox(0, i * size, size, size)
                val bitmap = LibRaw.getBitmap()
                Log.d("Test", "Tile: $i ${bitmap.width}x${bitmap.height}")
                list.add(bitmap)
            }
        }
        pfd.close()
        LibRaw.cleanup()
        return list
    }

    private fun unpackFd(pfd:ParcelFileDescriptor):Int {
        val fd = pfd.detachFd()
        val result = LibRaw.openFd(fd)
        if (result == 0) {
            LibRaw.setOutputBps(8) //Always 8 for Android
        }
        return result;
    }

    private fun getTile(top:Int, left:Int, width:Int, height:Int):Bitmap {
        LibRaw.setCropBox(top, left, width, height)
        return LibRaw.getBitmap()
    }

    private fun decodeFd(pfd:ParcelFileDescriptor): Bitmap? {
        val fd = pfd.detachFd()
        val bitmap = LibRaw.decodeAsBitmap(fd, true)
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