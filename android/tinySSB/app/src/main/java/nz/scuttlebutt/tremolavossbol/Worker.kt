import android.content.Context
import android.util.Log
import androidx.work.Worker
import androidx.work.WorkerParameters
import org.json.JSONObject
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class MyWorker(context: Context, workerParams: WorkerParameters) : Worker(context, workerParams) {
    override fun doWork(): Result {
        // Call the function you want to execute
        executeTask()

        // Indicate whether the task finished successfully with the Result
        return Result.success()
    }

    private fun executeTask() {
        Log.d("MyRealWorker", "Worker is working...")

        // Write the current time to output.txt
        val currentTime = System.currentTimeMillis()
        val formattedTime = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date(currentTime))
        applicationContext.openFileOutput("output.txt", Context.MODE_APPEND).use {
            it.write("$formattedTime\n".toByteArray())
        }

        // Check if contacts.json exists. If not, return.
        val contactsFile = File(applicationContext.filesDir, "contacts.json")
        if (!contactsFile.exists()) {
            Log.d("MyRealWorker", "contacts.json does not exist, returning...")
            return
        }

        // Read existing JSON from contacts.json
        val jsonString = applicationContext.openFileInput("contacts.json").bufferedReader().use { it.readText() }
        val json = if (jsonString.isNotEmpty()) JSONObject(jsonString) else JSONObject()

        // Prepare killList file in append mode
        val killListFile = applicationContext.openFileOutput("killList.txt", Context.MODE_APPEND)

        // Parse and check all keys, record which to remove
        val keys = json.keys()
        val keysToRemove = mutableListOf<String>()
        val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())

        while (keys.hasNext()) {
            val key = keys.next()
            val value = json.getString(key)
            // Check if the stored time is older than currentTime
            val contactTime = dateFormat.parse(value)?.time ?: 0L
            if (contactTime < currentTime) {
                keysToRemove.add(key)
            }
        }

        // Remove old keys and write them into killList
        keysToRemove.forEach { key ->
            killListFile.write("$key\n".toByteArray())
            json.remove(key)
        }
        killListFile.close()

        // Write updated JSON back to contacts.json (overwrite, not append)
        applicationContext.openFileOutput("contacts.json", Context.MODE_PRIVATE).use { out ->
            out.write(json.toString().toByteArray())
        }
    }
}