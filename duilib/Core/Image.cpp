#include "StdAfx.h"
#include "Image.h"

namespace ui 
{

ImageInfo::ImageInfo() :
	m_bAlphaChannel(false),
	m_bCached(false),
	m_propertyItem(),
	m_vecBitmap()
{

}

ImageInfo::~ImageInfo()
{
	for (auto it = m_vecBitmap.begin(); it != m_vecBitmap.end(); it++) {
		::DeleteObject(*it);
	}
}

void ImageInfo::SetPropertyItem(Gdiplus::PropertyItem* pPropertyItem)
{
	m_propertyItem.reset(pPropertyItem);
}

void ImageInfo::PushBackHBitmap(HBITMAP hBitmap)
{
	m_vecBitmap.push_back(hBitmap);
}

HBITMAP ImageInfo::GetHBitmap(int nIndex)
{
	return m_vecBitmap[nIndex];
}

int ImageInfo::GetFrameCount()
{
	return (int)m_vecBitmap.size();
}

bool ImageInfo::IsGif()
{
	return m_vecBitmap.size() > 1;
}

int ImageInfo::GetInterval(int nIndex)
{
	if (m_propertyItem == nullptr)
		return 0;

	if (nIndex >= (int)m_vecBitmap.size())
		return 0;

	int interval = ((long*)(m_propertyItem->value))[nIndex] * 10;
	if (interval < 30) {
		interval = 100;
	}
	else if (interval < 50)	{
		interval = 50;
	}
	return interval;
}

std::unique_ptr<ImageInfo> ImageInfo::LoadImage(const std::wstring& strImageFullPath)
{
	std::unique_ptr<Gdiplus::Bitmap> gdiplusBitmap(Gdiplus::Bitmap::FromFile(strImageFullPath.c_str()));
	return LoadImageByBitmap(gdiplusBitmap, strImageFullPath);
}

std::unique_ptr<ImageInfo> ImageInfo::LoadImage(HGLOBAL hGlobal, const std::wstring& imageFullPath)
{
	if (hGlobal == NULL)
	{
		return nullptr;
	}
	IStream* stream = NULL;
	GlobalLock(hGlobal);
	CreateStreamOnHGlobal(hGlobal, true, &stream);
	if (stream == NULL)
	{
		GlobalUnlock(hGlobal);
		return nullptr;
	}
	std::unique_ptr<Gdiplus::Bitmap> gdiplusBitmap(Gdiplus::Bitmap::FromStream(stream));
	stream->Release();
	GlobalUnlock(hGlobal);
	return LoadImageByBitmap(gdiplusBitmap, imageFullPath);
}

std::unique_ptr<ImageInfo> ImageInfo::LoadImageByBitmap(std::unique_ptr<Gdiplus::Bitmap>& pGdiplusBitmap, const std::wstring& strImageFullPath)
{
	Gdiplus::Status status;
	status = pGdiplusBitmap->GetLastStatus();
	ASSERT(status == Gdiplus::Ok);
	if (status != Gdiplus::Ok) {
		return nullptr;
	}

	UINT nCount = pGdiplusBitmap->GetFrameDimensionsCount();
	std::unique_ptr<GUID[]> pDimensionIDs(new GUID[nCount]);
	pGdiplusBitmap->GetFrameDimensionsList(pDimensionIDs.get(), nCount);
	int iFrameCount = pGdiplusBitmap->GetFrameCount(&pDimensionIDs.get()[0]);

	std::unique_ptr<ImageInfo> imageInfo(new ImageInfo);
	if (iFrameCount > 1) {
		int iSize = pGdiplusBitmap->GetPropertyItemSize(PropertyTagFrameDelay);
		Gdiplus::PropertyItem* pPropertyItem = (Gdiplus::PropertyItem*)malloc(iSize);
		status = pGdiplusBitmap->GetPropertyItem(PropertyTagFrameDelay, iSize, pPropertyItem);
		ASSERT(status == Gdiplus::Ok);
		if (status != Gdiplus::Ok) {
			return nullptr;
		}
		imageInfo->SetPropertyItem(pPropertyItem);
	}

	for (int i = 0; i < iFrameCount; i++) {
		status = pGdiplusBitmap->SelectActiveFrame(&Gdiplus::FrameDimensionTime, i);
		ASSERT(status == Gdiplus::Ok);
		if (status != Gdiplus::Ok) {
			return nullptr;
		}

		HBITMAP hBitmap;
		status = pGdiplusBitmap->GetHBITMAP(Gdiplus::Color(), &hBitmap);
		ASSERT(status == Gdiplus::Ok);
		if (status != Gdiplus::Ok) {
			return nullptr;
		}
		imageInfo->PushBackHBitmap(hBitmap);
	}

	imageInfo->nX = pGdiplusBitmap->GetWidth();
	imageInfo->nY = pGdiplusBitmap->GetHeight();
	imageInfo->sImageFullPath = strImageFullPath;
	Gdiplus::PixelFormat format = pGdiplusBitmap->GetPixelFormat();
	imageInfo->SetAlpha((format & PixelFormatAlpha) != 0);

	if ((format & PixelFormatIndexed) != 0) {
		int nPalSize = pGdiplusBitmap->GetPaletteSize();
		if (nPalSize > 0) {
			Gdiplus::ColorPalette *palette = (Gdiplus::ColorPalette*)malloc(nPalSize);;
			status = pGdiplusBitmap->GetPalette(palette, nPalSize);
			if (status == Gdiplus::Ok) {
				imageInfo->SetAlpha((palette->Flags & Gdiplus::PaletteFlagsHasAlpha) != 0);
			}
			free(palette);
		}
	}

	if (format == PixelFormat32bppARGB) {
		for (int nFrameIndex = 0; nFrameIndex < iFrameCount; nFrameIndex++) {
			HBITMAP hBitmap = imageInfo->GetHBitmap(nFrameIndex);
			BITMAP bm;
			::GetObject(hBitmap, sizeof(bm), &bm);
			LPBYTE imageBits = (LPBYTE)bm.bmBits;
			for (int i = 0; i < bm.bmHeight; ++i) {
				for (int j = 0; j < bm.bmWidthBytes; j += 4) {
					int x = i * bm.bmWidthBytes + j;
					if (imageBits[x + 3] != 255) {
						imageInfo->SetAlpha(true);
						return imageInfo;
					}
				}
			}
		}

		imageInfo->SetAlpha(false);
		return imageInfo;
	}

	return imageInfo;
}

ImageAttribute::ImageAttribute()
{
	Init();
}

void ImageAttribute::Init()
{
	simageString.clear();
	sImageName.clear();
	bFade = 0xFF;
	bTiledX = false;
	bTiledY = false;
	rcDest.left = rcDest.top = rcDest.right = rcDest.bottom = DUI_NOSET_VALUE;
	rcSource.left = rcSource.top = rcSource.right = rcSource.bottom = DUI_NOSET_VALUE;
	rcCorner.left = rcCorner.top = rcCorner.right = rcCorner.bottom = 0;
	nPlayCount = -1;
}

void ImageAttribute::SetImageString(const std::wstring& strImageString)
{
	Init();
	simageString = strImageString;
	sImageName = strImageString;
	ModifyAttribute(*this, strImageString);
}

void ImageAttribute::ModifyAttribute(ImageAttribute& imageAttribute, const std::wstring& strImageString)
{
	std::wstring sItem;
	std::wstring sValue;
	LPTSTR pstr = NULL;
	bool bScaleDest = true;

	LPCTSTR pStrImage = strImageString.c_str();
	while (*pStrImage != _T('\0')) {
		sItem.clear();
		sValue.clear();
		while (*pStrImage > _T('\0') && *pStrImage <= _T(' ')) pStrImage = ::CharNext(pStrImage);
		while (*pStrImage != _T('\0') && *pStrImage != _T('=') && *pStrImage > _T(' ')) {
			LPTSTR pstrTemp = ::CharNext(pStrImage);
			while (pStrImage < pstrTemp) {
				sItem += *pStrImage++;
			}
		}
		while (*pStrImage > _T('\0') && *pStrImage <= _T(' ')) pStrImage = ::CharNext(pStrImage);
		if (*pStrImage++ != _T('=')) break;
		while (*pStrImage > _T('\0') && *pStrImage <= _T(' ')) pStrImage = ::CharNext(pStrImage);
		if (*pStrImage++ != _T('\'')) break;
		while (*pStrImage != _T('\0') && *pStrImage != _T('\'')) {
			LPTSTR pstrTemp = ::CharNext(pStrImage);
			while (pStrImage < pstrTemp) {
				sValue += *pStrImage++;
			}
		}
		if (*pStrImage++ != _T('\'')) break;
		if (!sValue.empty()) {
			if (sItem == _T("file") || sItem == _T("res")) {
				imageAttribute.sImageName = sValue;
			}
			else if (sItem == _T("destscale")) {
				bScaleDest = (_tcscmp(sValue.c_str(), _T("true")) == 0);
			}
			else if (sItem == _T("dest")) {
				imageAttribute.rcDest.left = _tcstol(sValue.c_str(), &pstr, 10);  ASSERT(pstr);
				imageAttribute.rcDest.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);
				imageAttribute.rcDest.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);
				imageAttribute.rcDest.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);

				if (bScaleDest)
					DpiManager::GetInstance()->ScaleRect(imageAttribute.rcDest);
			}
			else if (sItem == _T("source")) {
				imageAttribute.rcSource.left = _tcstol(sValue.c_str(), &pstr, 10);  ASSERT(pstr);
				imageAttribute.rcSource.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);
				imageAttribute.rcSource.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);
				imageAttribute.rcSource.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);
			}
			else if (sItem == _T("corner")) {
				imageAttribute.rcCorner.left = _tcstol(sValue.c_str(), &pstr, 10);  ASSERT(pstr);
				imageAttribute.rcCorner.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);
				imageAttribute.rcCorner.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);
				imageAttribute.rcCorner.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);
			}
			else if (sItem == _T("fade")) {
				imageAttribute.bFade = (BYTE)_tcstoul(sValue.c_str(), &pstr, 10);
			}
			else if (sItem == _T("xtiled")) {
				imageAttribute.bTiledX = (_tcscmp(sValue.c_str(), _T("true")) == 0);
			}
			else if (sItem == _T("ytiled")) {
				imageAttribute.bTiledY = (_tcscmp(sValue.c_str(), _T("true")) == 0);
			}
			else if (sItem == _T("playcount"))
			{
				imageAttribute.nPlayCount = _tcstol(sValue.c_str(), &pstr, 10);  ASSERT(pstr);
			}
		}
		if (*pStrImage++ != _T(' ')) break;
	}
}


Image::Image() :
	imageAttribute(),
	imageCache(),
	m_nCurrentFrame(0),
	m_bPlaying(false),
	m_nCycledCount(0)
{

}

void Image::SetImageString(const std::wstring& strImageString)
{
	ClearCache();
	imageAttribute.SetImageString(strImageString);
}

void Image::ClearCache()
{
	m_nCurrentFrame = 0;
	m_bPlaying = false;
	imageCache.reset();
	m_nCycledCount = 0;
}

bool Image::IncrementCurrentFrame()
{
	if (!imageCache) {
		return false;
	}
	m_nCurrentFrame++;
	if (m_nCurrentFrame == imageCache->GetFrameCount()) {
		m_nCurrentFrame = 0;
		m_nCycledCount += 1;
	}
	return true;
}

void Image::SetCurrentFrame(int nCurrentFrame)
{
	m_nCurrentFrame = nCurrentFrame;
}

HBITMAP Image::GetCurrentHBitmap()
{
	if (!imageCache) {
		return NULL;
	}
	return imageCache->GetHBitmap(m_nCurrentFrame);
}

int Image::GetCurrentInterval()
{
	if (!imageCache) {
		return 0;
	}
	return imageCache->GetInterval(m_nCurrentFrame);
}
int Image::GetCurrentFrameIndex()
{
	return m_nCurrentFrame;
}
int Image::GetCycledCount()
{
	return m_nCycledCount;
}
void Image::ClearCycledCount()
{
	m_nCycledCount = 0;
}
bool Image::ContinuePlay()
{
	if (imageAttribute.nPlayCount < 0)
		return true;
	else if (imageAttribute.nPlayCount == 0)
		return m_bPlaying;
	else
		return m_nCycledCount < imageAttribute.nPlayCount;
}
StateImage::StateImage() :
	m_pControl(nullptr),
	m_stateImageMap()
{

}

bool StateImage::HasHotImage()
{
	return !m_stateImageMap[kControlStateHot].imageAttribute.simageString.empty();
}

bool StateImage::PaintStatusImage(IRenderContext* pRender, ControlStateType stateType, const std::wstring& sImageModify /*= L""*/)
{
	if (m_pControl) {
		bool bFadeHot = m_pControl->GetAnimationManager().GetAnimationPlayer(kAnimationHot) != nullptr;
		int nHotAlpha = m_pControl->GetHotAlpha();
		if (bFadeHot) {
			if (stateType == kControlStateNormal || stateType == kControlStateHot) {
				std::wstring strNormalImagePath = m_stateImageMap[kControlStateNormal].imageAttribute.sImageName;
				std::wstring strHotImagePath = m_stateImageMap[kControlStateHot].imageAttribute.sImageName;

				if (strNormalImagePath.empty() || strHotImagePath.empty()
					|| strNormalImagePath != strHotImagePath
					|| !m_stateImageMap[kControlStateNormal].imageAttribute.rcSource.Equal(m_stateImageMap[kControlStateHot].imageAttribute.rcSource)) {

					m_pControl->DrawImage(pRender, m_stateImageMap[kControlStateNormal], sImageModify);
					int nHotFade = m_stateImageMap[kControlStateHot].imageAttribute.bFade;
					nHotFade = int(nHotFade * (double)nHotAlpha / 255);
					return m_pControl->DrawImage(pRender, m_stateImageMap[kControlStateHot], sImageModify, nHotFade);
				}
				else {
					int nNormalFade = m_stateImageMap[kControlStateNormal].imageAttribute.bFade;
					int nHotFade = m_stateImageMap[kControlStateHot].imageAttribute.bFade;
					int nBlendFade = int((1 - (double)nHotAlpha / 255) * nNormalFade + (double)nHotAlpha / 255 * nHotFade);
					return m_pControl->DrawImage(pRender, m_stateImageMap[kControlStateHot], sImageModify, nBlendFade);
				}
			}
		}
	}

	if (stateType == kControlStatePushed && m_stateImageMap[kControlStatePushed].imageAttribute.simageString.empty()) {
		stateType = kControlStateHot;
		m_stateImageMap[kControlStateHot].imageAttribute.bFade = 255;
	}
	if (stateType == kControlStateHot && m_stateImageMap[kControlStateHot].imageAttribute.simageString.empty()) {
		stateType = kControlStateNormal;
	}
	if (stateType == kControlStateDisabled && m_stateImageMap[kControlStateDisabled].imageAttribute.simageString.empty()) {
		stateType = kControlStateNormal;
	}

	return m_pControl->DrawImage(pRender, m_stateImageMap[stateType], sImageModify);
}

Image* StateImage::GetEstimateImage()
{
	Image* pEstimateImage = nullptr;
	if (!m_stateImageMap[kControlStateNormal].imageAttribute.sImageName.empty()){
		pEstimateImage = &m_stateImageMap[kControlStateNormal];
	}
	else if (!m_stateImageMap[kControlStateHot].imageAttribute.sImageName.empty()) {
		pEstimateImage = &m_stateImageMap[kControlStateHot];
	}
	else if (!m_stateImageMap[kControlStatePushed].imageAttribute.sImageName.empty()) {
		pEstimateImage = &m_stateImageMap[kControlStatePushed];
	}
	else if (!m_stateImageMap[kControlStateDisabled].imageAttribute.sImageName.empty()) {
		pEstimateImage = &m_stateImageMap[kControlStateDisabled];
	}

	return pEstimateImage;
}

void StateImage::ClearCache()
{
	auto it = m_stateImageMap.find(kControlStateNormal);
	if (it != m_stateImageMap.end())
	{
		it->second.ClearCache();
	}
	it = m_stateImageMap.find(kControlStateHot);
	if (it != m_stateImageMap.end())
	{
		it->second.ClearCache();
	}
	it = m_stateImageMap.find(kControlStatePushed);
	if (it != m_stateImageMap.end())
	{
		it->second.ClearCache();
	}
	it = m_stateImageMap.find(kControlStateDisabled);
	if (it != m_stateImageMap.end())
	{
		it->second.ClearCache();
	}
}


void StateImageMap::SetControl(Control* control)
{
	m_stateImageMap[kStateImageBk].SetControl(control);
	m_stateImageMap[kStateImageFore].SetControl(control);
	m_stateImageMap[kStateImageSelectedBk].SetControl(control);
	m_stateImageMap[kStateImageSelectedFore].SetControl(control);
}

void StateImageMap::SetImage(StateImageType stateImageType, ControlStateType stateType, const std::wstring& strImagePath)
{
	m_stateImageMap[stateImageType][stateType].SetImageString(strImagePath);
}

std::wstring StateImageMap::GetImagePath(StateImageType stateImageType, ControlStateType stateType)
{
	return m_stateImageMap[stateImageType][stateType].imageAttribute.simageString;
}

bool StateImageMap::HasHotImage()
{
	for (auto& it : m_stateImageMap) {
		if (it.second.HasHotImage()) {
			return true;
		}
	}
	return false;
}

bool StateImageMap::PaintStatusImage(IRenderContext* pRender, StateImageType stateImageType, ControlStateType stateType, const std::wstring& sImageModify /*= L""*/)
{
	auto it = m_stateImageMap.find(stateImageType);
	if (it != m_stateImageMap.end()) {
		return it->second.PaintStatusImage(pRender, stateType, sImageModify);
	}
	return false;
}

Image* StateImageMap::GetEstimateImage(StateImageType stateImageType)
{
	auto it = m_stateImageMap.find(stateImageType);
	if (it != m_stateImageMap.end()) {
		return it->second.GetEstimateImage();
	}
	return nullptr;
}

void StateImageMap::ClearCache()
{
	m_stateImageMap[kStateImageBk].ClearCache();
	m_stateImageMap[kStateImageFore].ClearCache();
	m_stateImageMap[kStateImageSelectedBk].ClearCache();
	m_stateImageMap[kStateImageSelectedFore].ClearCache();
}


StateColorMap::StateColorMap() :
	m_pControl(nullptr),
	m_stateColorMap()
{
}

void StateColorMap::SetControl(Control* control)
{
	m_pControl = control;
}

bool StateColorMap::HasHotColor()
{
	return !m_stateColorMap[kControlStateHot].empty();
}

void StateColorMap::PaintStatusBorderColor(IRenderContext* pRender, UiRect rcItem, UiRect rcBorderSize, ControlStateType stateType)
{
	/*if (m_pControl) {
		bool bFadeHot = m_pControl->GetAnimationManager().GetAnimationPlayer(kAnimationHot) != nullptr;
		int nHotAlpha = m_pControl->GetHotAlpha();
		if (bFadeHot) {
			if ((stateType == kControlStateNormal || stateType == kControlStateHot)
				&& !m_stateColorMap[kControlStateHot].empty()) {
				PaintBorder(pRender,rcItem,rcBorderSize,kControlStateNormal);
				if (nHotAlpha > 0) {
					PaintBorder(pRender,rcItem,rcBorderSize,kControlStateHot);
				}
				return;
			}
		}
	}*/

	if (stateType == kControlStatePushed && m_stateColorMap[kControlStatePushed].empty()) {
		stateType = kControlStateHot;
	}
	if (stateType == kControlStateHot && m_stateColorMap[kControlStateHot].empty()) {
		stateType = kControlStateNormal;
	}
	if (stateType == kControlStateDisabled && m_stateColorMap[kControlStateDisabled].empty()) {
		stateType = kControlStateNormal;
	}
	PaintBorder(pRender,rcItem,rcBorderSize,stateType);
}


void StateColorMap::PaintBorder(IRenderContext* pRender, UiRect rcItem, UiRect rcBorderSize, ControlStateType stateType)
{
	if (m_stateColorMap.count(stateType) < 1)
		return;

	DWORD dwBorderColor = 0;
	dwBorderColor = GlobalManager::GetTextColor(m_stateColorMap[stateType]);

	if (dwBorderColor != 0) {
		if (rcBorderSize.left > 0 || rcBorderSize.top > 0 || rcBorderSize.right > 0 || rcBorderSize.bottom > 0) {
			UiRect rcBorder;
			if (rcBorderSize.left > 0) {
				rcBorder = rcItem;
				rcBorder.right = rcBorder.left = rcItem.left + rcBorderSize.left / 2;
				if (rcBorderSize.left == 1) {
					rcBorder.bottom -= 1;
				}
				pRender->DrawLine(rcBorder, rcBorderSize.left, dwBorderColor);
			}
			if (rcBorderSize.top > 0) {
				rcBorder = rcItem;
				rcBorder.bottom = rcBorder.top = rcItem.top + rcBorderSize.top / 2;
				if (rcBorderSize.top == 1) {
					rcBorder.right -= 1;
				}
				pRender->DrawLine(rcBorder, rcBorderSize.top, dwBorderColor);
			}
			if (rcBorderSize.right > 0) {
				rcBorder = rcItem;
				rcBorder.left = rcBorder.right = rcItem.right - (rcBorderSize.right + 1) / 2;
				if (rcBorderSize.right == 1) {
					rcBorder.bottom -= 1;
				}
				pRender->DrawLine(rcBorder, rcBorderSize.right, dwBorderColor);
			}
			if (rcBorderSize.bottom > 0) {
				rcBorder = rcItem;
				rcBorder.top = rcBorder.bottom = rcItem.bottom - (rcBorderSize.bottom + 1) / 2;
				if (rcBorderSize.bottom == 1) {
					rcBorder.right -= 1;
				}
				pRender->DrawLine(rcBorder, rcBorderSize.bottom, dwBorderColor);
			}
		}
		/*else if (m_nBorderSize > 0) {
			UiRect rcDraw = rcItem;
			int nDeltaValue = m_nBorderSize / 2;
			rcDraw.top += nDeltaValue;
			rcDraw.bottom -= nDeltaValue;
			if (m_nBorderSize % 2 != 0) {
				rcDraw.bottom -= 1;
			}
			rcDraw.left += nDeltaValue;
			rcDraw.right -= nDeltaValue;
			if (m_nBorderSize % 2 != 0) {
				rcDraw.right -= 1;
			}

			if (m_cxyBorderRound.cx > 0 || m_cxyBorderRound.cy > 0)
				pRender->DrawRoundRect(rcDraw, m_cxyBorderRound, m_nBorderSize, dwBorderColor);
			else
				pRender->DrawRect(rcDraw, m_nBorderSize, dwBorderColor);
		}*/
	}
}

void StateColorMap::PaintStatusColor(IRenderContext* pRender, UiRect rcPaint, ControlStateType stateType)
{
	if (m_pControl) {
		bool bFadeHot = m_pControl->GetAnimationManager().GetAnimationPlayer(kAnimationHot) != nullptr;
		int nHotAlpha = m_pControl->GetHotAlpha();
		if (bFadeHot) {
			if ((stateType == kControlStateNormal || stateType == kControlStateHot)
				&& !m_stateColorMap[kControlStateHot].empty()) {

				pRender->DrawColor(rcPaint, m_stateColorMap[kControlStateNormal]);
				if (nHotAlpha > 0) {
					pRender->DrawColor(rcPaint, m_stateColorMap[kControlStateHot], nHotAlpha);
				}
				return;
			}
		}
	}

	if (stateType == kControlStatePushed && m_stateColorMap[kControlStatePushed].empty()) {
		stateType = kControlStateHot;
	}
	if (stateType == kControlStateHot && m_stateColorMap[kControlStateHot].empty()) {
		stateType = kControlStateNormal;
	}
	if (stateType == kControlStateDisabled && m_stateColorMap[kControlStateDisabled].empty()) {
		stateType = kControlStateNormal;
	}

	pRender->DrawColor(rcPaint, m_stateColorMap[stateType]);
}

}