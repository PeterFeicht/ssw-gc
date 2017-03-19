/**
 * @file    RestoreStream.hpp
 * @author  niob
 * @date    Jan 24, 2017
 * @brief   Defines the {@link RestoreStream} class for automatically restoring stream settings.
 */

#ifndef RESTORESTREAM_HPP_
#define RESTORESTREAM_HPP_

namespace ssw {

/**
 * @brief Provides a way to reset settings for a stream automatically.
 * 
 * To use this class, simply construct an instance with automatic storage duration, specifying the stream
 * you want to be reset. As soon as the instance is destroyed, settings for the stream are restored to the
 * values they had on construction.
 * Of course you may also destroy the instance manually, that's just not as convenient.
 * 
 * Managed settings are:
 *  - flags
 *  - precision
 *  - width
 */
class RestoreStream final
{
	std::ios_base &mStream;
	const std::ios_base::fmtflags mFlags;
	const std::streamsize mWidth;
	const std::streamsize mPrecision;
	
public:
	
	/** Initialize a new RestoreStream object for the specified stream. */
	explicit RestoreStream(std::ios_base &stream) noexcept :
			mStream(stream), mFlags(stream.flags()), mWidth(stream.width()), mPrecision(stream.precision()) {
	}
	
	/** Destroy this instance (reset the stream to its initial configuration). */
	~RestoreStream() noexcept {
		mStream.flags(mFlags);
		mStream.width(mWidth);
		mStream.precision(mPrecision);
	}
	
	RestoreStream(const RestoreStream&) = delete;
	RestoreStream& operator=(const RestoreStream&) = delete;
	RestoreStream(RestoreStream&&) = delete;
	RestoreStream& operator=(RestoreStream&&) = delete;
	
	/** Get the flags that will be set */
	auto flags() const noexcept {
		return mFlags;
	}
	/** Get the width that will be set */
	auto width() const noexcept {
		return mWidth;
	}
	/** Get the precision that will be set */
	auto precision() const noexcept {
		return mPrecision;
	}
}; // class RestoreStream

} // namespace ssw

#endif /* RESTORESTREAM_HPP_ */
