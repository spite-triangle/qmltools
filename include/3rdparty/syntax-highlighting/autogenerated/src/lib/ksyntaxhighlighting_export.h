
#ifndef KSYNTAXHIGHLIGHTING_EXPORT_H
#define KSYNTAXHIGHLIGHTING_EXPORT_H

#ifdef KSYNTAXHIGHLIGHTING_STATIC_DEFINE
#  define KSYNTAXHIGHLIGHTING_EXPORT
#  define KSYNTAXHIGHLIGHTING_NO_EXPORT
#else
#  ifndef KSYNTAXHIGHLIGHTING_EXPORT
#    ifdef KF6SyntaxHighlighting_EXPORTS
        /* We are building this library */
#      define KSYNTAXHIGHLIGHTING_EXPORT Q_DECL_EXPORT
#    else
        /* We are using this library */
#      define KSYNTAXHIGHLIGHTING_EXPORT Q_DECL_IMPORT
#    endif
#  endif

#  ifndef KSYNTAXHIGHLIGHTING_NO_EXPORT
#    define KSYNTAXHIGHLIGHTING_NO_EXPORT 
#  endif
#endif

#ifndef KSYNTAXHIGHLIGHTING_DECL_DEPRECATED
#  define KSYNTAXHIGHLIGHTING_DECL_DEPRECATED __declspec(deprecated)
#endif

#ifndef KSYNTAXHIGHLIGHTING_DECL_DEPRECATED_EXPORT
#  define KSYNTAXHIGHLIGHTING_DECL_DEPRECATED_EXPORT KSYNTAXHIGHLIGHTING_EXPORT KSYNTAXHIGHLIGHTING_DECL_DEPRECATED
#endif

#ifndef KSYNTAXHIGHLIGHTING_DECL_DEPRECATED_NO_EXPORT
#  define KSYNTAXHIGHLIGHTING_DECL_DEPRECATED_NO_EXPORT KSYNTAXHIGHLIGHTING_NO_EXPORT KSYNTAXHIGHLIGHTING_DECL_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef KSYNTAXHIGHLIGHTING_NO_DEPRECATED
#    define KSYNTAXHIGHLIGHTING_NO_DEPRECATED
#  endif
#endif

#define KSYNTAXHIGHLIGHTING_DECL_DEPRECATED_TEXT(text) __declspec(deprecated(text))

/* Take any defaults from group settings */
#if !defined(KSYNTAXHIGHLIGHTING_NO_DEPRECATED) && !defined(KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT)
#  ifdef KF_NO_DEPRECATED
#    define KSYNTAXHIGHLIGHTING_NO_DEPRECATED
#  elif defined(KF_DISABLE_DEPRECATED_BEFORE_AND_AT)
#    define KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT KF_DISABLE_DEPRECATED_BEFORE_AND_AT
#  endif
#endif
#if !defined(KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT) && defined(KF_DISABLE_DEPRECATED_BEFORE_AND_AT)
#  define KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT KF_DISABLE_DEPRECATED_BEFORE_AND_AT
#endif

#if !defined(KSYNTAXHIGHLIGHTING_NO_DEPRECATED_WARNINGS) && !defined(KSYNTAXHIGHLIGHTING_DEPRECATED_WARNINGS_SINCE)
#  ifdef KF_NO_DEPRECATED_WARNINGS
#    define KSYNTAXHIGHLIGHTING_NO_DEPRECATED_WARNINGS
#  elif defined(KF_DEPRECATED_WARNINGS_SINCE)
#    define KSYNTAXHIGHLIGHTING_DEPRECATED_WARNINGS_SINCE KF_DEPRECATED_WARNINGS_SINCE
#  endif
#endif
#if !defined(KSYNTAXHIGHLIGHTING_DEPRECATED_WARNINGS_SINCE) && defined(KF_DEPRECATED_WARNINGS_SINCE)
#  define KSYNTAXHIGHLIGHTING_DEPRECATED_WARNINGS_SINCE KF_DEPRECATED_WARNINGS_SINCE
#endif

#if defined(KSYNTAXHIGHLIGHTING_NO_DEPRECATED)
#  undef KSYNTAXHIGHLIGHTING_DEPRECATED
#  define KSYNTAXHIGHLIGHTING_DEPRECATED_EXPORT KSYNTAXHIGHLIGHTING_EXPORT
#  define KSYNTAXHIGHLIGHTING_DEPRECATED_NO_EXPORT KSYNTAXHIGHLIGHTING_NO_EXPORT
#elif defined(KSYNTAXHIGHLIGHTING_NO_DEPRECATED_WARNINGS)
#  define KSYNTAXHIGHLIGHTING_DEPRECATED
#  define KSYNTAXHIGHLIGHTING_DEPRECATED_EXPORT KSYNTAXHIGHLIGHTING_EXPORT
#  define KSYNTAXHIGHLIGHTING_DEPRECATED_NO_EXPORT KSYNTAXHIGHLIGHTING_NO_EXPORT
#else
#  define KSYNTAXHIGHLIGHTING_DEPRECATED KSYNTAXHIGHLIGHTING_DECL_DEPRECATED
#  define KSYNTAXHIGHLIGHTING_DEPRECATED_EXPORT KSYNTAXHIGHLIGHTING_DECL_DEPRECATED_EXPORT
#  define KSYNTAXHIGHLIGHTING_DEPRECATED_NO_EXPORT KSYNTAXHIGHLIGHTING_DECL_DEPRECATED_NO_EXPORT
#endif

/* No deprecated API had been removed from build */
#define KSYNTAXHIGHLIGHTING_EXCLUDE_DEPRECATED_BEFORE_AND_AT 0

#define KSYNTAXHIGHLIGHTING_BUILD_DEPRECATED_SINCE(major, minor) 1

#ifdef KSYNTAXHIGHLIGHTING_NO_DEPRECATED
#  define KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT KSYNTAXHIGHLIGHTING_VERSION
#endif
#ifdef KSYNTAXHIGHLIGHTING_NO_DEPRECATED_WARNINGS
#  define KSYNTAXHIGHLIGHTING_DEPRECATED_WARNINGS_SINCE 0
#endif

#ifndef KSYNTAXHIGHLIGHTING_DEPRECATED_WARNINGS_SINCE
#  ifdef KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT
#    define KSYNTAXHIGHLIGHTING_DEPRECATED_WARNINGS_SINCE KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT
#  else
#    define KSYNTAXHIGHLIGHTING_DEPRECATED_WARNINGS_SINCE KSYNTAXHIGHLIGHTING_VERSION
#  endif
#endif

#ifndef KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT
#  define KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT 0
#endif

#ifdef KSYNTAXHIGHLIGHTING_DEPRECATED
#  define KSYNTAXHIGHLIGHTING_ENABLE_DEPRECATED_SINCE(major, minor) (((major<<16)|(minor<<8)) > KSYNTAXHIGHLIGHTING_DISABLE_DEPRECATED_BEFORE_AND_AT)
#else
#  define KSYNTAXHIGHLIGHTING_ENABLE_DEPRECATED_SINCE(major, minor) 0
#endif

#endif /* KSYNTAXHIGHLIGHTING_EXPORT_H */