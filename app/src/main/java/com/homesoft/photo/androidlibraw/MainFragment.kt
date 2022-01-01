package com.homesoft.photo.androidlibraw

import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import android.graphics.drawable.BitmapDrawable
import android.media.ExifInterface
import android.os.*
import android.system.Os
import android.system.OsConstants
import android.util.Log
import android.view.*
import androidx.fragment.app.Fragment
import android.widget.ImageView
import android.widget.ProgressBar
import android.widget.Toast
import androidx.annotation.StringRes
import androidx.appcompat.app.AppCompatActivity
import com.homesoft.photo.libraw.LibRaw
import java.lang.Exception
import java.util.concurrent.Executors
import androidx.preference.PreferenceManager
import com.homesoft.photo.util.ImageUtil


/**
 * The main [Fragment] subclass.
 */
class MainFragment : Fragment() {
    private val executor = Executors.newSingleThreadExecutor()
    private lateinit var raw: ImageView
    private lateinit var hdr: ImageView
    private lateinit var jpeg: ImageView
    lateinit var progressBar: ProgressBar

    private val handler: Handler = object : Handler(Looper.getMainLooper()) {
        override fun handleMessage(msg: Message) {
            Toast.makeText(requireActivity(), msg.what, Toast.LENGTH_LONG).show()
            setBusy(false)
        }
    }


    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        // Inflate the layout for this fragment
        val view = inflater.inflate(R.layout.fragment_main, container, false)
        raw = view.findViewById(R.id.raw)
        hdr = view.findViewById(R.id.hdr)
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            hdr.visibility = View.GONE
            view.findViewById<View>(R.id.hdrLabel).visibility = View.GONE
        }
        jpeg = view.findViewById(R.id.jpeg)
        progressBar = view.findViewById(R.id.progressBar)
        setHasOptionsMenu(true)
        return view
    }

    override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater) {
        inflater.inflate(R.menu.main_menu, menu)

    }

    override fun onStart() {
        super.onStart()
        (requireActivity() as AppCompatActivity).supportActionBar?.let {
            it.setDisplayHomeAsUpEnabled(false)
            it.title = getString(R.string.app_name)
        }
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        // Handle item selection
        return when (item.itemId) {
            R.id.open -> {
                select()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }

    private fun setDefaultImage(imageView: ImageView) {
        val layoutParams = imageView.layoutParams
        layoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT
        layoutParams.height = ViewGroup.LayoutParams.WRAP_CONTENT
        val drawable = imageView.drawable
        imageView.setImageResource(R.drawable.ic_baseline_photo_camera_24)
        if (drawable is BitmapDrawable) {
            drawable.bitmap?.recycle()
        }
        imageView.layoutParams = layoutParams
    }

    private fun setRawImage(imageView: ImageView, bitmap: Bitmap?, viewWidth: Int) {
        bitmap?.let {
            val scale = viewWidth / bitmap.width.toFloat()
            val layoutParams = imageView.layoutParams
            layoutParams.width = viewWidth
            layoutParams.height = (bitmap.height * scale).toInt()
            imageView.setImageBitmap(bitmap)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == PICK_FILE
            && resultCode == AppCompatActivity.RESULT_OK
        ) {
            // The result data contains a URI for the document or directory that
            // the user selected.
            if (data != null) {
                jpeg.imageMatrix.reset()
                setDefaultImage(jpeg)
                setDefaultImage(raw)
                setDefaultImage(hdr)
                setBusy(true)
                data.data?.let { uri ->
                    val activity = requireActivity()
                    val contentResolver = activity.contentResolver
                    // Perform operations on the document using its URI.
                    executor.execute {
                        try {
                            val viewWidth = view?.width ?: return@execute
                            val pfd = contentResolver.openFileDescriptor(
                                uri, "r", null
                            )
                            if (pfd == null) {
                                showError(R.string.open_failed)
                                return@execute
                            }
                            //val bitmap = decodeMemory(pfd)
                            val jpegBitmap = decodeEmbeddedJpeg(pfd, viewWidth)

                            val opts = BitmapFactory.Options()
                            val rawBitmap: Bitmap?
                            val hdrBitmap: Bitmap?
                            val libRaw = openFd(pfd, opts, viewWidth)
                            val orientation: Int
                            if (libRaw != null) {
                                libRaw.use {
                                    orientation = it.orientation
                                    opts.inPreferredConfig = Bitmap.Config.ARGB_8888
                                    rawBitmap = it.decodeAsBitmap(opts)
                                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                                        opts.inPreferredConfig = Bitmap.Config.RGBA_F16
                                        hdrBitmap = it.decodeAsBitmap(opts)
                                    } else {
                                        hdrBitmap = null
                                    }
                                }
                            } else {
                                rawBitmap = null
                                hdrBitmap = null
                                orientation = 0
                            }
                            val jpegMatrix = Matrix()
                            jpegBitmap?.let {
                                val degrees = ImageUtil.getDegrees(orientation)
                                val scale = viewWidth / if (degrees == 0 || degrees == 180) {
                                    jpegBitmap.width.toFloat()
                                } else {
                                    jpegBitmap.height.toFloat()
                                }
                                val rotationPoint = ImageUtil.getRotationalPoint(it.width, it.height, degrees)
                                jpegMatrix.setRotate(degrees.toFloat(), rotationPoint.x, rotationPoint.y)
                                jpegMatrix.postScale(scale, scale)
                            }
                            activity.runOnUiThread {
                                setBusy(false)
                                setRawImage(raw, rawBitmap, viewWidth)
                                setRawImage(hdr, hdrBitmap, viewWidth)
                                jpegBitmap?.let {
                                    val layoutParams = jpeg.layoutParams
                                    layoutParams.width = viewWidth
                                    val array = floatArrayOf(it.width.toFloat(), it.height.toFloat())
                                    jpegMatrix.mapPoints(array)
                                    layoutParams.height = array[1].toInt()
                                    jpeg.imageMatrix = jpegMatrix
                                    jpeg.setImageBitmap(jpegBitmap)
                                    jpeg.layoutParams = layoutParams
                                }
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

    override fun onDestroy() {
        super.onDestroy()
        executor.shutdownNow()
    }

    private fun setBusy(busy:Boolean) {
        if (busy) {
            progressBar.visibility = View.VISIBLE
        } else {
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

    /**
     * Decoded using the JPEG embedded in the RAW
     */
    private fun decodeEmbeddedJpeg(pfd: ParcelFileDescriptor, viewWidth: Int): Bitmap? {
        val options = BitmapFactory.Options()
        options.inJustDecodeBounds
        BitmapFactory.decodeFileDescriptor(pfd.fileDescriptor, null, options)
        Log.d("Test", "Size ${options.outWidth}x${options.outHeight}")
        options.inSampleSize = getSampleSize(viewWidth, options.outWidth)
        val bitmap = BitmapFactory.decodeFileDescriptor(pfd.fileDescriptor, null, options)
        return bitmap
    }


    private fun decodeTiles(pfd: ParcelFileDescriptor):List<Bitmap> {
        val list = mutableListOf<Bitmap>()
        val libRaw = unpackFd(pfd)
        pfd.close()
        libRaw?.use {
            it.setQuality(2)
            val size = 256
            for (i in 0..8) {
                it.setCropBox(0, i * size, size, size)
                val bitmap = it.bitmap
                Log.d("Test", "Tile: $i ${bitmap.width}x${bitmap.height}")
                list.add(bitmap)
            }
        }
        return list
    }

    private fun unpackFd(pfd: ParcelFileDescriptor):LibRaw? {
        val fd = pfd.detachFd()
        val libRaw = LibRaw()
        val result = libRaw.openFd(fd)
        if (result == 0) {
            libRaw.setOutputBps(8) //Always 8 for Android
            return libRaw
        }
        libRaw.close()
        return null
    }

//    private fun getTile(left:Int, top:Int, width:Int, height:Int): Bitmap {
//        LibRaw.setCropBox(left, top, width, height)
//        return LibRaw.getBitmap()
//    }

    private fun getSampleSize(viewWidth:Int, imageWidth:Int):Int {
        var sampleSize = 1
        var scaledImageWidth = imageWidth
        while (scaledImageWidth > 2 * viewWidth) {
            sampleSize *=2
            scaledImageWidth /=2
        }
        return sampleSize
    }

    private fun openFd(pfd: ParcelFileDescriptor, opts:BitmapFactory.Options, viewWidth:Int):LibRaw? {
        val context = requireContext()
        val prefs = PreferenceManager.getDefaultSharedPreferences(context)
        val autoWhiteBalance = prefs.getBoolean("autoWhiteBalance", context.resources.getBoolean(R.bool.defaultAutoWhiteBalance))
        val libRaw = LibRaw()
        libRaw.setAutoWhiteBalance(autoWhiteBalance)
        val colorSpace = prefs.getString("colorSpace", context.resources.getString(R.string.defaultColorSpace))
        libRaw.setOutputColorSpace(colorSpace!!.toInt())
        val fd = pfd.detachFd()
        val result = libRaw.openFd(fd)
        pfd.close()
        if (result != 0) {
            libRaw.close()
            return null
        }
        val orientation = libRaw.orientation
        val width = if (orientation == ExifInterface.ORIENTATION_ROTATE_90 || orientation == ExifInterface.ORIENTATION_ROTATE_270) {libRaw.height} else {libRaw.width}

        opts.inSampleSize = getSampleSize(viewWidth, width)
        return libRaw
    }

    /**
     * Loads the file entirely into
     * May be faster
     */
    private fun decodeMemory(pfd: ParcelFileDescriptor): Bitmap? {
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
            return null
        }
        val opts = BitmapFactory.Options()
        opts.inSampleSize = 2
        val bitmap = LibRaw.decodeAsBitmap(buffer, structStat.st_size.toInt(), opts)
        Os.munmap(buffer, structStat.st_size)
        pfd.close()
        return bitmap
    }

    companion object {
        const val PICK_FILE = 1234
    }
}