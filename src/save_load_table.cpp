#include <boost/text/save_load_table.hpp>

#define BOOST_TEXT_USE_XML_ARCHIVES_FOR_TESTING 0

#if BOOST_TEXT_USE_XML_ARCHIVES_FOR_TESTING
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#else
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#endif
#include <boost/serialization/array.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/vector.hpp>


namespace boost { namespace serialization {

    namespace text_dtl {
        template<typename T>
        boost::serialization::nvp<T> const nvp(const char * name, T & t)
        {
            return boost::serialization::nvp<T>(name, t);
        }
    }

    template<typename Ar>
    void serialize(
        Ar & ar,
        boost::text::v1::detail::collation_element & ce,
        unsigned const version)
    {
        ar & text_dtl::nvp("l1", ce.l1_);
        ar & text_dtl::nvp("l2", ce.l2_);
        ar & text_dtl::nvp("l3", ce.l3_);
        ar & text_dtl::nvp("l4", ce.l4_);
    }

    template<typename Ar>
    void serialize(
        Ar & ar,
        boost::text::v1::detail::collation_elements & ces,
        unsigned const version)
    {
        ar & text_dtl::nvp("first", ces.first_);
        ar & text_dtl::nvp("last", ces.last_);
        ar & text_dtl::nvp("lead_primary", ces.lead_primary_);
        ar & text_dtl::nvp("lead_primary_shifted", ces.lead_primary_shifted_);
    }

    template<typename Ar, int N>
    void serialize(
        Ar & ar,
        boost::text::v1::detail::collation_trie_key<N> & key,
        unsigned const version)
    {
        static_assert(N < (1 << 7), "That value of N is unsupported.");
        int8_t size = key.size_;
        BOOST_ASSERT(0 <= size && size <= N);
        ar & text_dtl::nvp("size", size);
        BOOST_ASSERT(0 <= size && size <= N);
        for (int i = 0; i < size; ++i) {
            ar & text_dtl::nvp("value", key.cps_.values_[i]);
        }
        key.size_ = size;
    }

    template<typename Ar>
    void serialize(
        Ar & ar,
        boost::text::v1::detail::nonsimple_script_reorder & reorder,
        unsigned const version)
    {
        ar & text_dtl::nvp("first", reorder.first_);
        ar & text_dtl::nvp("last", reorder.last_);
        ar & text_dtl::nvp("lead_byte", reorder.lead_byte_);
    }

}}

namespace boost { namespace text { inline namespace v1 {

#if BOOST_TEXT_USE_XML_ARCHIVES_FOR_TESTING
    using oarchive_type = boost::archive::xml_oarchive;
    using iarchive_type = boost::archive::xml_iarchive;
#else
    using oarchive_type = boost::archive::binary_oarchive;
    using iarchive_type = boost::archive::binary_iarchive;
#endif

    namespace detail {
        template<typename T>
        boost::serialization::nvp<T> const nvp(char const * name, T & t)
        {
            return boost::serialization::nvp<T>(name, t);
        }
    }

    bool save_table(
        collation_table const & table_proper, filesystem::path const & path)
    try {
        auto const & table = *table_proper.data_;
        filesystem::ofstream ofs(path, std::ios_base::binary);
        if (!ofs)
            return false;
        oarchive_type ar(ofs);

        detail::collation_trie_t::trie_map_type trie_map(table.trie_.impl_);

        std::ptrdiff_t trie_size = trie_map.size();
        BOOST_ASSERT(
            trie_size == std::distance(trie_map.begin(), trie_map.end()));
        ar & detail::nvp("trie_size", trie_size);
        for (auto const & kv : trie_map) {
            ar & detail::nvp("key", kv.key);
            ar & detail::nvp("value", kv.value);
        }

        ar & detail::nvp("collation_element_vec", table.collation_element_vec_);

        ar & detail::nvp("nonstarter_first", table.nonstarter_first_);
        ar & detail::nvp("nonstarter_last", table.nonstarter_last_);
        ar & detail::nvp("nonstarter_table", table.nonstarter_table_);

        std::ptrdiff_t nonsimple_reorders_size =
            table.nonsimple_reorders_.size();
        ar & detail::nvp("nonsimple_reorders_size", nonsimple_reorders_size);
        for (auto const & e : table.nonsimple_reorders_) {
            ar & detail::nvp("e", e);
        }
        ar & detail::nvp("simple_reorders", table.simple_reorders_);

        ar & detail::nvp("strength", table.strength_);
        ar & detail::nvp("weighting", table.weighting_);
        ar & detail::nvp("l2_order", table.l2_order_);
        ar & detail::nvp("case_level", table.case_level_);
        ar & detail::nvp("case_first", table.case_first_);

        return true;
    } catch (...) {
        return false;
    }

    collation_table load_table(filesystem::path const & path)
    {
        collation_table retval;

        auto & table = *retval.data_;
        std::ifstream ifs(path.string(), std::ios_base::binary);
        if (!ifs) {
            throw invalid_table(
                "Could not open file at the path given to load_table().");
        }
        iarchive_type ar(ifs);
        ifs.tellg();

        std::ptrdiff_t trie_size = 0;
        ar & detail::nvp("trie_size", trie_size);
        if (trie_size <= 0)
            throw invalid_table("Saved table trie has <= 0 elements.");

        try {
            for (std::ptrdiff_t i = 0; i < trie_size; ++i) {
                detail::collation_trie_key<32> key;
                detail::collation_elements value;
                ar & detail::nvp("key", key);
                ar & detail::nvp("value", value);
                table.trie_.insert(key, value);
            }

            ar & detail::nvp(
                "collation_element_vec", table.collation_element_vec_);
            if (table.collation_element_vec_.empty())
                table.collation_elements_ = detail::collation_elements_ptr();

            ar & detail::nvp("nonstarter_first", table.nonstarter_first_);
            ar & detail::nvp("nonstarter_;ast", table.nonstarter_last_);
            ar & detail::nvp("nonstarter_table", table.nonstarter_table_);
            if (retval.data_->nonstarter_table_.empty())
                table.nonstarters_ = detail::default_table_nonstarters_ptr();

            std::ptrdiff_t nonsimple_reorders_size = 0;
            ar & detail::nvp("nonsimple_reorders", nonsimple_reorders_size);
            for (std::ptrdiff_t i = 0; i < nonsimple_reorders_size; ++i) {
                detail::nonsimple_script_reorder e;
                ar & detail::nvp("e", e);
                table.nonsimple_reorders_.push_back(e);
            }
            ar & detail::nvp("simple_reorders", table.simple_reorders_);

            ar & detail::nvp("strength", table.strength_);
            ar & detail::nvp("weighting", table.weighting_);
            ar & detail::nvp("l2_order", table.l2_order_);
            ar & detail::nvp("case_level", table.case_level_);
            ar & detail::nvp("case_first", table.case_first_);

        } catch (...) {
            throw invalid_table("Invalid serialized table data.");
        }

        return retval;
    }

}}}
