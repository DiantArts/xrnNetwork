#pragma once

namespace xrn::network::detail::constraint {

///////////////////////////////////////////////////////////////////////////
/// \brief Checks whether the type contains a last value
///
/// \tparam Type to check
///
/// \return True if the Type given as template parameter contains a
///         Type::last
///
///////////////////////////////////////////////////////////////////////////
template <
    typename Type
> concept hasValueLast = requires(Type){ Type::last; };

///////////////////////////////////////////////////////////////////////////
/// \brief Checks whether the type contains a last value and is an Enum
///
/// \tparam Type to check
///
/// \return True if the Type given as template parameter contains a
///         Type::last and is an Enum
///
/// \see ::xrn::network::detail::contraint::hasValueLast
///
///////////////////////////////////////////////////////////////////////////
template <
    typename Type
> concept isValidEnum =
    ::std::is_enum<Type>::value && ::xrn::network::detail::constraint::hasValueLast<Type>;

} // namespace xrn::network::detail::constraint
