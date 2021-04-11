/******************************************/
/*  HbFont - Hb Function Wrap             */
/*  ver: 0.1                              */
/*  Copyright (C) 2021,cnlog              */
/******************************************/

#ifndef UI_GFX_HB_FONT_H_
#define UI_GFX_HB_FONT_H_

#include "include/core/SkTypes.h"

#ifdef SK_BUILD_FOR_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#    define WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#    define NOMINMAX_WAS_LOCALLY_DEFINED
#    undef  min
#    undef  max
#  endif
#
#  include <windows.h>
#
#  ifdef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#    undef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#    undef WIN32_LEAN_AND_MEAN
#  endif
#  ifdef NOMINMAX_WAS_LOCALLY_DEFINED
#    undef NOMINMAX_WAS_LOCALLY_DEFINED
#    undef NOMINMAX
#  endif
#endif //SK_BUILD_FOR_WIN

#ifndef DCHECK
#define DCHECK(x) (void)0 
#endif

#include "include/core/SkSurface.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkPath.h"
#include "include/core/SkColorPriv.h"
#include "include/core/SkUnPreMultiply.h"
#include "include/private/SkMutex.h"
#include "include/private/SkTFitsIn.h"
#include "include/third_party/harfbuzz/hb.h"

//#include "SkUTF.h"

#include <uchar.h>
#include <icu.h>
#include <codecvt>
#include <stdexcept>
#include <queue>
#include "mru_cache.h"

#include <iostream>
#include <cassert>

typedef int32_t UChar32;

#if defined(_DEBUG) && defined(_WIN32)
#   define _CRTDBG_MAP_ALLOC
#   include <Cstdlib>
#   include <crtdbg.h>
    inline void EnableMemLeakCheck()
    {
        _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    };
#   define _traceX(fmt, ...)         \
    {                                \
        TCHAR buffer[512] = { 0 };   \
        va_list args;                \
        va_start(args, fmt);         \
        wsprintf(buffer, fmt, args); \
        va_end(args);                \
        OutputDebugString(buffer);   \
    };
    //#define new   new(_NORMAL_BLOCK, __FILE__, __LINE__)
#   define TRACE _trace
#   define DumpMemLeaks()      {  _CrtDumpMemoryLeaks();          }
#   define _trace(fmt,...)     {  SkDebugf( fmt, ##__VA_ARGS__ ); }
#   define _trace0(x)          {  SkDebugf( "%s\r\n", x );        }
#   define SK_DEBUG
#else
#   define SK_RELEASE
#   define _trace(fmt,...)     (void)0 
#   define _traceX(fmt,...)    (void)0
#   define _trace0(x) (void)0 
    inline void EnableMemLeakCheck1(){}
#   define DumpMemLeaks() (void)0
#endif // _DEBUG

#define NOCOPY(T)	T(const T&); T& operator=(const T&);

#define throwException(message)                      \
    {                                                \
        std::ostringstream _ost;                     \
        _ost << __FILE __ << " " << __LINE__ << " "  \
             << __FUNC__  << " " << message;         \
        throw std::exception(_ost.str().c_str());    \
    }

// namespace gfx
namespace gfx {

template<typename Type>
void DeleteByType(void* data) {
    Type* typed_data = reinterpret_cast<Type*>(data);
    delete typed_data;
};

template<typename Type>
    void DeleteArrayByType(void* data) {
    Type* typed_data = reinterpret_cast<Type*>(data);
    delete[] typed_data;
};

    constexpr int  kDefaultFontNumber = 13;
    constexpr auto kDefaultFontColor  = SK_ColorBLACK;
    constexpr auto kDefaultBkColor    = SK_ColorGRAY;
    constexpr auto kDefaultFont = "Microsoft YaHei";
    constexpr int  kTypefaceCacheSize = 64;
    constexpr int  kGlyphCacheSize    = 1024;

    using u8string=std::string ;

    template <typename T,typename P,P* p> using resource = std::unique_ptr<T, SkFunctionWrapper<P, p>>;
    using HBBlob   = resource<hb_blob_t     , decltype(hb_blob_destroy)  , hb_blob_destroy  >;
    using HBFace   = resource<hb_face_t     , decltype(hb_face_destroy)  , hb_face_destroy  >;
    using HBFont   = resource<hb_font_t     , decltype(hb_font_destroy)  , hb_font_destroy  >;
    using HBBuffer = resource<hb_buffer_t   , decltype(hb_buffer_destroy), hb_buffer_destroy>;
    using HBSet    = resource<hb_set_t      , decltype(hb_set_destroy)   , hb_set_destroy   >;
   // using HBSubSet = resource<hb_subset_input_t, decltype(hb_subset_input_destroy), hb_subset_input_destroy>;

    class  TypefaceData;

    using  GlyphCache       = base::MRUCache<hb_codepoint_t, SkGlyphID>;
    using  TypefaceCache    = base::MRUCache<SkFontID, TypefaceData>;

    using  TypefaceCachePtr = std::unique_ptr<TypefaceCache>;
    using  GlyphCachePtr    = std::unique_ptr<GlyphCache>;

    static SkMutex g_faceCacheMutex;
    static TypefaceCachePtr g_face_caches = std::make_unique<TypefaceCache>(kTypefaceCacheSize);

    struct FontData {
        explicit FontData(GlyphCachePtr& glyph_cache):glyph_cache_(glyph_cache){}
        SkFont font_;
        GlyphCachePtr &glyph_cache_;
    }; //struct FontData

    // Outputs the |width| and |extents| of the glyph with index |codepoint| in
    // |paint|'s font.
    // We treat HarfBuzz ints as 16.16 fixed-point.
    static __forceinline int SkiaScalarToHarfBuzzUnits(SkScalar v) {
        return SkScalarRoundToInt(v);
    };
    static void GetGlyphWidthAndExtents(const SkFont& font,
                                hb_codepoint_t codepoint,
                                hb_position_t* width,
                                hb_glyph_extents_t* extents) {
            //DCHECK_LE(codepoint, std::numeric_limits<uint16_t>::max());

            SkScalar sk_width;
            SkRect   sk_bounds;
            uint16_t glyph = static_cast<uint16_t>(codepoint);

            font.getWidths(&glyph, 1, &sk_width, &sk_bounds);
            if (width)
                *width = SkiaScalarToHarfBuzzUnits(sk_width);
            if (extents) {
                // Invert y-axis because Skia is y-grows-down but we set up HarfBuzz to be
                // y-grows-up.
                extents->x_bearing = SkiaScalarToHarfBuzzUnits( sk_bounds.fLeft);
                extents->y_bearing = SkiaScalarToHarfBuzzUnits(-sk_bounds.fTop);
                extents->width     = SkiaScalarToHarfBuzzUnits( sk_bounds.width());
                extents->height    = SkiaScalarToHarfBuzzUnits(-sk_bounds.height());
            }
    };

    // Writes the |glyph| index for the given |unicode| code point. Returns whether
    // the glyph exists, i.e. it is not a missing glyph.
    static hb_bool_t HbGetGlyph(hb_font_t* font,
                                void* data,
                                hb_codepoint_t  unicode,
                                hb_codepoint_t  variation_selector,
                                hb_codepoint_t* glyph,
                                void* user_data) _Acquires_exclusive_lock_(g_faceCacheMutex) {
        SkAutoMutexExclusive lock(g_faceCacheMutex);
        FontData* font_data = reinterpret_cast<FontData*>(data);
        GlyphCache *cache = font_data->glyph_cache_.get();
        GlyphCache::iterator iter = cache->Get(unicode);
        if (iter == cache->end()) {
            auto result = cache->Put(unicode, font_data->font_.unicharToGlyph(unicode));
            iter = result;
        }

        *glyph = iter->second;
        return !!*glyph;
    }; //GetGlyph

    static hb_bool_t HbGetNominalGlyph(hb_font_t* font,
                                    void* data,
                                    hb_codepoint_t unicode,
                                    hb_codepoint_t* glyph,
                                    void* user_data) {
            return HbGetGlyph(font, data, unicode, 0, glyph, user_data);
    }; //GetNominalGlyph

    // Returns the horizontal advance value of the |glyph|.
    static hb_position_t HbGetGlyphHorizontalAdvance(hb_font_t* font,
                                    void* data,
                                    hb_codepoint_t glyph,
                                    void* user_data) {
        FontData* font_data = reinterpret_cast<FontData*>(data);
        hb_position_t advance = 0;

        GetGlyphWidthAndExtents(font_data->font_, glyph, &advance, 0);
        return advance;
    };

    static hb_bool_t HbGetGlyphHorizontalOrigin(hb_font_t* font,
                                    void* data,
                                    hb_codepoint_t glyph,
                                    hb_position_t* x,
                                    hb_position_t* y,
                                    void* user_data) {
        // Just return true, like the HarfBuzz-FreeType implementation.
        return true;
    };

    static hb_position_t HbGetGlyphKerning(FontData* font_data,
                                    hb_codepoint_t first_glyph,
                                    hb_codepoint_t second_glyph) {
        SkTypeface* typeface = font_data->font_.getTypeface();
        const uint16_t glyphs[2] = { static_cast<uint16_t>(first_glyph),
                                     static_cast<uint16_t>(second_glyph) };
        int32_t kerning_adjustments[1] = { 0 };

        if (!typeface->getKerningPairAdjustments(glyphs, 2, kerning_adjustments))
            return 0;

        SkScalar upm = SkIntToScalar(typeface->getUnitsPerEm());
        SkScalar size = font_data->font_.getSize();
        return SkiaScalarToHarfBuzzUnits(SkIntToScalar(kerning_adjustments[0]) * size / upm);
    };

    static hb_position_t HbGetGlyphHorizontalKerning(hb_font_t* font,
                                        void* data,
                                        hb_codepoint_t left_glyph,
                                        hb_codepoint_t right_glyph,
                                        void* user_data) {
        FontData* font_data = reinterpret_cast<FontData*>(data);
        return HbGetGlyphKerning(font_data, left_glyph, right_glyph);
    };

    static hb_position_t HbGetGlyphVerticalKerning(hb_font_t* font,
                                        void* data,
                                        hb_codepoint_t top_glyph,
                                        hb_codepoint_t bottom_glyph,
                                        void* user_data) {
        FontData* font_data = reinterpret_cast<FontData*>(data);
        return HbGetGlyphKerning(font_data, top_glyph, bottom_glyph);
    };    
    // Writes the |extents| of |glyph|.
    static hb_bool_t HbGetGlyphExtents( hb_font_t* font,
                                        void* data,
                                        hb_codepoint_t glyph,
                                        hb_glyph_extents_t* extents,
                                        void* user_data) {
        FontData* font_data = reinterpret_cast<FontData*>(data);

        GetGlyphWidthAndExtents(font_data->font_, glyph, 0, extents);
        return true;
    };

    static hb_font_funcs_t* HbGetFontFuncs() {
        static hb_font_funcs_t* const funcs = []{
            // HarfBuzz will use the default (parent) implementation if they aren't set.
            hb_font_funcs_t* const funcs = hb_font_funcs_create();
            hb_font_funcs_set_variation_glyph_func(
                    funcs, HbGetGlyph, nullptr, nullptr);
            hb_font_funcs_set_nominal_glyph_func(
                    funcs, HbGetNominalGlyph, nullptr, nullptr);

            hb_font_funcs_set_glyph_h_advance_func(
                    funcs, HbGetGlyphHorizontalAdvance, nullptr, nullptr);
            hb_font_funcs_set_glyph_h_kerning_func(
                    funcs, HbGetGlyphHorizontalKerning, nullptr, nullptr);
            hb_font_funcs_set_glyph_h_origin_func(
                    funcs, HbGetGlyphHorizontalOrigin, nullptr, nullptr);
            hb_font_funcs_set_glyph_v_kerning_func(
                    funcs, HbGetGlyphVerticalKerning, nullptr, nullptr);
            hb_font_funcs_set_glyph_extents_func(
                    funcs, HbGetGlyphExtents, nullptr, nullptr);

            hb_font_funcs_make_immutable(funcs);
            return funcs;
        }();
        SkASSERT(funcs);
        return funcs;
    }; //skhb_get_font_funcs

    // Returns the raw data of the font table |tag|.
    static hb_blob_t* HbGetFontTable(hb_face_t* face, hb_tag_t tag, void* context) {
            SkTypeface&  typeface   = *reinterpret_cast<SkTypeface*>(context);
            auto data = typeface.copyTableData(tag);
            if (!data) {
                return nullptr;
            }
            SkData* rawData = data.release();
            return hb_blob_create(reinterpret_cast<char*>(rawData->writable_data()), rawData->size(),
                        HB_MEMORY_MODE_READONLY, rawData, [](void* ctx) {
                            SkSafeUnref(((SkData*)ctx)); }
                    );
    }; //GetFontTable

    //class TypefaceData
    class TypefaceData {
    public:
        explicit TypefaceData(sk_sp<SkTypeface> skia_face) :sk_typeface_(skia_face) ,
                              glyphs_( std::make_unique<GlyphCache>(kGlyphCacheSize) ) {
                sk_typeface_->ref();
                face_.reset(hb_face_create_for_tables(HbGetFontTable, 
                    const_cast<SkTypeface*>(sk_typeface_.get()),
                    [](void* user_data) { SkSafeUnref(reinterpret_cast<SkTypeface*>(user_data)); }));
        }
        TypefaceData(TypefaceData&& data) {
                face_   = std::move(data.face_) ;
                glyphs_ = std::move(data.glyphs_);
                sk_typeface_ = std::move(data.sk_typeface_);
                data.face_   = nullptr;
                data.glyphs_ = nullptr;
                data.sk_typeface_ = nullptr;
        }
        ~TypefaceData() { /*hb_face_destroy(face_);*/ 
            face_  = nullptr;
            glyphs_= nullptr;
            sk_typeface_= nullptr;
        }
    public:
        HBFace   &face()               { return face_;        }
        GlyphCachePtr& glyphs()        { return glyphs_;      }
        sk_sp<SkTypeface> &typeface()  { return sk_typeface_; }
        void clearAll() {
            face_.reset();
            glyphs_.reset();
            sk_typeface_.reset();
        }
    private:
        TypefaceData() = delete;
        GlyphCachePtr  glyphs_;
        HBFace face_;
        sk_sp<SkTypeface> sk_typeface_;
    }; //class TypefaceData

    //class TextShaper
    class TextShaper{
        public:
            TextShaper(u8string language):language_(language){}
            ~TextShaper(){
                {
                   /* SkAutoMutexExclusive lock(g_faceCacheMutex);
                    for (TypefaceCache::iterator it = g_face_caches->begin(); it != g_face_caches->end(); ++it) {
                        //it->second.clearAll();
                        int b0 = it->second.typeface() == nullptr ? -1 : it->second.typeface()->uniqueID();
                        int b1 = (it->second.face() == nullptr) ? -1 : -2;
                        int b2 = it->second.glyphs() == nullptr ? -1 : (it->second.glyphs()->size());
                        _trace("~TextShaper== b0:%d b1:%d b2:%d\r\n", b0, b1, b2);
                    }
                    */
                }
                _trace0("~TextShaper");
            }
        public:
            void SetLanguage(u8string str) { 
                language_=str; 
            }
            sk_sp<SkTextBlob> Shape(const char* text,sk_sp<SkTypeface> skia_face ,SkFont skfont){
                    HBBuffer buffer(hb_buffer_create());
                    if(!buffer){
                        _trace0("Could not create hb_buffer");
                        return nullptr;
                    }
                    hb_buffer_t *buf=buffer.get();
                    //SkAutoTCallVProc<hb_buffer_t, hb_buffer_clear_contents> autoClearBuffer(buf);
                    hb_buffer_set_content_type(buf, HB_BUFFER_CONTENT_TYPE_UNICODE);
                    hb_buffer_set_cluster_level(buf,HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS );
                    hb_buffer_add_utf8(buf, text, -1, 0, -1);                    
                    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);  
                    hb_buffer_set_script(buf, HB_SCRIPT_COMMON);  
                    //hb_buffer_set_script(buf, HB_SCRIPT_INHERITED);
                    //hb_language_get_default is not thread safe
                    hb_language_t hbLanguage = hb_language_from_string(language_.c_str(), -1); 
                    if (hbLanguage == HB_LANGUAGE_INVALID) {
                        _trace0("HB_LANGUAGE_INVALID");
                        hbLanguage = hb_language_from_string("und", -1);
                    }
                    hb_buffer_set_language(buf, hbLanguage);   
                    hb_buffer_guess_segment_properties(buf);
                    HBFont hb_font(CreateHbFont(skia_face,skfont.getSize()));
                    //assert(hb_font);
                    if (!hb_font) {
                        _trace0("Could not create hb_font");
                        return nullptr;
                    }
                    hb_shape(hb_font.get(), buf, nullptr, 0);
                    unsigned int glyph_count;
                    hb_glyph_info_t *glyph_info    = hb_buffer_get_glyph_infos(buf, &glyph_count);
                    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count); 

                    SkTextBlobBuilder builder;
                    const auto& runBuffer = builder.allocRunPos(skfont, glyph_count);  
                    float x = 0;
                    float y = 0;
                    for (unsigned int i = 0; i < glyph_count; i++) {
                        runBuffer.glyphs[i] = glyph_info[i].codepoint;
                        reinterpret_cast<SkPoint*>(runBuffer.pos)[i] = SkPoint::Make(
                                x + glyph_pos[i].x_offset ,
                                y - glyph_pos[i].y_offset );
                        x += glyph_pos[i].x_advance;
                        y += glyph_pos[i].y_advance; 
                    }  
                    return builder.make();           
            }
        public:
           // Creates a HarfBuzz font from the given Skia face and text size/font size.
            HBFont CreateHbFont(sk_sp<SkTypeface> skia_face,
                                SkScalar text_size) _Acquires_exclusive_lock_(g_faceCacheMutex) {
                SkAutoMutexExclusive lock(g_faceCacheMutex);
                // A cache from Skia font to harfbuzz typeface information.
                TypefaceCache* skFaceCached = g_face_caches.get();
                assert(skFaceCached);
                TypefaceCache::iterator skFaceData = skFaceCached->Get(skia_face->uniqueID());
                if (skFaceData == skFaceCached->end()) {
                    TypefaceData new_skFaceData(skia_face);
                    skFaceData = skFaceCached->Put(skia_face->uniqueID(),std::move(new_skFaceData));
                }
                DCHECK(skFaceData->second.face());
                HBFace& hb_face = skFaceData->second.face();
                if (!hb_face) {
                    _trace0("Could not create hb_face");
                    return nullptr;
                }
                hb_face_set_upem(hb_face.get(), skFaceData->second.typeface()->getUnitsPerEm());
                HBFont hb_font_parent( hb_font_create(hb_face.get()) );
                if(!hb_font_parent){
                    _trace0("Could not create hb_font");
                    return nullptr;
                }
                // Creating a sub font means that non-available functions
                // are found from the parent.
                HBFont hb_font(hb_font_create_sub_font(hb_font_parent.get()));
                if (!hb_font) {
                    _trace0("Could not create hb_font");
                    return nullptr;
                }
                int nSize = SkScalarRoundToInt(text_size);
                hb_font_set_scale(hb_font.get(), nSize, nSize);
                FontData* hb_font_data = new FontData(skFaceData->second.glyphs());
                if (!hb_font_data) {
                    _trace0("Could not new hb_font_data");
                    return nullptr;
                }
                hb_font_data->font_.setTypeface(skFaceData->second.typeface());
                hb_font_data->font_.setSize(text_size);
                hb_font_set_funcs(hb_font.get(), HbGetFontFuncs(), 
                                  hb_font_data,DeleteByType<FontData>);
                hb_font_make_immutable(hb_font.get());
                return hb_font;
            }
        private:
            u8string language_;
            NOCOPY(TextShaper);

    }; // class TextShaper
 
} // namespace gfx	

#endif // UI_GFX_HB_FONT_H_