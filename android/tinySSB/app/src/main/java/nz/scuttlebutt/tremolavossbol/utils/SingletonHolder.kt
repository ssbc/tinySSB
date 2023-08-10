package nz.scuttlebutt.tremolavossbol.utils

//https://stackoverflow.com/questions/40398072/singleton-with-parameter-in-kotlin

open class SingletonHolder<out T, in A>(private var creator: (A) -> T ) {
    @Volatile
    private var instance: T? = null

    fun getInstance(arg: A): T {
        return when {
            instance != null -> instance!!
            else -> synchronized(this) {
                if (instance == null) instance = creator(arg)
                instance!!
            }
        }
    }
}