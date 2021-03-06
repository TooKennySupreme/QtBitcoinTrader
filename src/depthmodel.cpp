// Copyright (C) 2013 July IGHOR.
// I want to create trading application that can be configured for any rule and strategy.
// If you want to help me please Donate: 1d6iMwjjNo8ZGYeJBZKXgcgVk9o7fXcjc
// For any questions please use contact form https://sourceforge.net/projects/bitcointrader/
// Or send e-mail directly to julyighor@gmail.com
//
// You may use, distribute and copy the Qt Bitcion Trader under the terms of
// GNU General Public License version 3

#include "depthmodel.h"
#include "main.h"
#include <QTimer>

DepthModel::DepthModel(bool isAskData)
	: QAbstractItemModel()
{
	widthPriceTitle=75;
	widthVolumeTitle=75;
	widthSizeTitle=75;

	groupedPrice=0.0;
	groupedVolume=0.0;
	widthPrice=75;
	widthVolume=75;
	widthSize=75;
	somethingChanged=true;
	isAsk=isAskData;
	originalIsAsk=isAsk;
	columnsCount=5;
}

DepthModel::~DepthModel()
{

}

int DepthModel::rowCount(const QModelIndex &) const
{
	return priceList.count()+grouped;
}

int DepthModel::columnCount(const QModelIndex &) const
{
	return columnsCount;
}

QVariant DepthModel::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())return QVariant();
	int currentRow=index.row();

	if(role==Qt::WhatsThisRole)
	{
		return baseValues.currentPair.currBSign+priceListStr.at(currentRow)+" "+baseValues.currentPair.currASign+volumeListStr.at(currentRow)+" "+baseValues.currentPair.currASign+sizeListStr.at(currentRow);
	}

	if(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole&&role!=Qt::ForegroundRole&&role!=Qt::BackgroundRole&&role!=Qt::TextAlignmentRole)return QVariant();

	int indexColumn=index.column();
	if(isAsk)indexColumn=columnsCount-indexColumn-1;

	if(role==Qt::TextAlignmentRole)
	{
		if(indexColumn==0)return 0x0081;
		if(indexColumn==1)return 0x0082;
		if(indexColumn==3)return 0x0082;
		return 0x0084;
	}

	if(grouped&&currentRow<2)
	{
		if(role==Qt::ForegroundRole)return QVariant();
		if(currentRow==1||groupedPrice==0.0)return QVariant();
		QString firstRowText;
		switch(indexColumn)
		{
		case 0: //Price
				firstRowText=mainWindow.numFromDouble(groupedPrice);
				if(role==Qt::ToolTipRole)firstRowText.prepend(baseValues.currentPair.currBSign);	
				break; 
		case 1: //Volume
				firstRowText=QString::number(groupedVolume,'f',baseValues.currentPair.currADecimals);  
				if(role==Qt::ToolTipRole)firstRowText.prepend(baseValues.currentPair.currASign);
				break;
		}
		if(firstRowText.isEmpty())return QVariant();
		return firstRowText;
	}

	if(grouped)currentRow-=grouped;
	if(currentRow<0||currentRow>=priceList.count())return QVariant();

	if(!originalIsAsk)currentRow=priceList.count()-currentRow-1;

	if(role==Qt::ForegroundRole)
	{
		if(indexColumn==1)
		{
		double volume=volumeList.at(currentRow);
		if(volume<1.0)
		{
			if(baseValues.appTheme.nightMode) return QColor(255,255,255,255-(155+volume*100.0)).lighter(200).lighter(200);
			else return QColor(0,0,0,155+volume*100.0);
		}
		else if(volume<100.0)return QVariant();
		else if(volume<1000.0)return baseValues.appTheme.blue;
		else return baseValues.appTheme.red;
		}
		return QVariant();
	}

	double requestedPrice=priceList.at(currentRow);
	if(requestedPrice<=0.0)return QVariant();

	if(role==Qt::BackgroundRole)
	{
		if(originalIsAsk)
		{
			if(mainWindow.ordersModel->currentAsksPrices.value(requestedPrice,false))return baseValues.appTheme.lightGreen;
		}
		else
		{
			if(mainWindow.ordersModel->currentBidsPrices.value(requestedPrice,false))return baseValues.appTheme.lightGreen;
		}
		return QVariant();
	}

	QString returnText;

	switch(indexColumn)
	{
	case 0://Price
		if(role==Qt::ToolTipRole)baseValues.currentPair.currBSign+priceListStr.at(currentRow);
		return priceListStr.at(currentRow);
		break;
	case 1:
		{//Volume
		if(volumeList.at(currentRow)<=0.0)return QVariant();
		if(role==Qt::ToolTipRole)baseValues.currentPair.currASign+volumeListStr.at(currentRow);
		return volumeListStr.at(currentRow);
		}
		break;
	case 2:
		{//Direction
			switch(directionList.at(currentRow))
			{
			case -1: return downArrowStr;
			case 1: return upArrowStr;
			default: return QVariant();
			}
		}
	case 3:
		{//Size
		if(sizeList.at(currentRow)<=0.0)return QVariant();
		if(role==Qt::ToolTipRole)baseValues.currentPair.currASign+sizeListStr.at(currentRow);
		return sizeListStr.at(currentRow);
		}
		break;
	default: break;
	}
	if(!returnText.isEmpty())return returnText;
	return QVariant();
}

void DepthModel::reloadVisibleItems()
{
	QTimer::singleShot(100,this,SLOT(delayedReloadVisibleItems()));
}

void DepthModel::delayedReloadVisibleItems()
{
	emit dataChanged(index(0,0),index(priceList.count()-1,columnsCount-1));
}

void DepthModel::calculateSize()
{
	if(!somethingChanged)return;
	somethingChanged=true;

	double maxPrice=0.0;
	double maxVolume=0.0;
	double maxTotal=0.0;

	double totalSize=0.0;
	if(originalIsAsk)
	{
		for(int n=0;n<priceList.count();n++)
		{
			int currentRow=n;
			if(!originalIsAsk)currentRow=priceList.count()-currentRow-1;

			totalSize+=volumeList.at(currentRow);
			sizeList[currentRow]=totalSize;
			sizeListStr[currentRow]=QString::number(totalSize,'f',baseValues.currentPair.currADecimals);

			maxPrice=qMax(maxPrice,priceList.at(currentRow));
			maxVolume=qMax(maxVolume,volumeList.at(currentRow));
			maxTotal=qMax(maxTotal,sizeList.at(currentRow));
		}
	}
	else
	{
		for(int n=priceList.count()-1;n>=0;n--)
		{
			int currentRow=n;
			if(originalIsAsk)currentRow=priceList.count()-currentRow-1;
			totalSize+=volumeList.at(currentRow);
			sizeList[currentRow]=totalSize;
			sizeListStr[currentRow]=QString::number(totalSize,'f',baseValues.currentPair.currADecimals);

			maxPrice=qMax(maxPrice,priceList.at(currentRow));
			maxVolume=qMax(maxVolume,volumeList.at(currentRow));
			maxTotal=qMax(maxTotal,sizeList.at(currentRow));
		}
	}

	widthPrice=10+textFontWidth(mainWindow.numFromDouble(maxPrice,baseValues.currentPair.priceDecimals));
	widthVolume=10+textFontWidth(QString::number(maxVolume,'f',baseValues.currentPair.currADecimals));
	widthSize=10+textFontWidth(QString::number(maxTotal,'f',baseValues.currentPair.currADecimals));
	
	widthPrice=qMax(widthPrice,widthPriceTitle);
	widthVolume=qMax(widthVolume,widthVolumeTitle);
	widthSize=qMax(widthSize,widthSizeTitle);

	int sizeColumn=2;
	if(isAsk)sizeColumn=1;
	emit dataChanged(index(0,sizeColumn),index(priceList.count()-1,sizeColumn));
}

QModelIndex DepthModel::index(int row, int column, const QModelIndex &parent) const
{
	if(!hasIndex(row, column, parent))return QModelIndex();
	return createIndex(row,column);
}

QModelIndex DepthModel::parent(const QModelIndex &) const
{
	return QModelIndex();
}

void DepthModel::clear()
{
	if(priceList.isEmpty())return;
	beginResetModel();
	groupedPrice=0.0;
	groupedVolume=0.0;
	directionList.clear();
	priceList.clear();
	volumeList.clear();
	sizeList.clear();
	priceListStr.clear();
	volumeListStr.clear();
	sizeListStr.clear();
	endResetModel();
	somethingChanged=false;
}

Qt::ItemFlags DepthModel::flags(const QModelIndex &index) const
{
	if(!index.isValid())return 0;
	if(grouped)
	{
		if(index.row()==1||groupedPrice==0.0&&priceList.isEmpty())return Qt::NoItemFlags;
	}
	return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}

QVariant DepthModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	int indexColumn=section;
	if(isAsk)indexColumn=columnsCount-indexColumn-1;

	if(orientation!=Qt::Horizontal)return QVariant();

	if(role==Qt::TextAlignmentRole)
	{
		if(indexColumn==0)return 0x0081;
		if(indexColumn==1)return 0x0082;
		if(indexColumn==2)return 0x0082;
		return 0x0084;
	}

	if(role==Qt::SizeHintRole)
	{
		switch(indexColumn)
		{
		case 0: return QSize(widthPrice,defaultHeightForRow);//Price
		case 1: return QSize(widthVolume,defaultHeightForRow);//Volume
		case 3: return QSize(widthSize,defaultHeightForRow);//Size
		}
		return QVariant();
	}

	if(role!=Qt::DisplayRole)return QVariant();
	if(headerLabels.count()!=columnsCount)return QVariant();

	switch(indexColumn)
	{
	case 0: return headerLabels.at(indexColumn)+QLatin1String(" ")+baseValues.currentPair.currBSign;
	case 1: return headerLabels.at(indexColumn)+QLatin1String(" ")+baseValues.currentPair.currASign;
	case 3: return headerLabels.at(indexColumn)+QLatin1String(" ")+baseValues.currentPair.currASign;
	}

	return headerLabels.at(indexColumn);
}

void DepthModel::fixTitleWidths()
{
	int curASize=textFontWidth(" "+baseValues.currentPair.currASign);
	int curBSize=textFontWidth(" "+baseValues.currentPair.currBSign);
	widthPriceTitle=textFontWidth(headerLabels.at(0))+20+curBSize;
	widthVolumeTitle=textFontWidth(headerLabels.at(1))+20+curASize;
	widthSizeTitle=textFontWidth(headerLabels.at(3))+20+curASize;
}

void DepthModel::setHorizontalHeaderLabels(QStringList list)
{
	if(list.count()!=columnsCount)return;
	headerLabels=list;
	fixTitleWidths();
	emit headerDataChanged(Qt::Horizontal, 0, columnsCount-1);
}

void DepthModel::depthFirstOrder(double price, double volume)
{
	if(!grouped)return;
	if(price==groupedPrice&&groupedVolume==volume)return;
	groupedPrice=price;
	groupedVolume=volume;
	if(isAsk)
		emit dataChanged(index(0,3),index(0,4));
	else 
		emit dataChanged(index(0,0),index(0,1));
}

void DepthModel::depthUpdateOrders(QList<DepthItem> *items)
{
	if(items==0)return;
	for(int n=0;n<items->count();n++)depthUpdateOrder(items->at(n));
	delete items;
	calculateSize();
}

void DepthModel::depthUpdateOrder(DepthItem item)
{
	double price=item.price;
	double volume=item.volume;
	if(price==0.0)return;
	int currentIndex=qLowerBound(priceList.begin(),priceList.end(),price)-priceList.begin();
	bool matchListRang=currentIndex>-1&&priceList.count()>currentIndex;

	if(volume==0.0)
	{//Remove item
		if(matchListRang)
		{
			beginRemoveRows(QModelIndex(), currentIndex+grouped, currentIndex+grouped);
			directionList.removeAt(currentIndex);
			priceList.removeAt(currentIndex);
			volumeList.removeAt(currentIndex);
			sizeList.removeAt(currentIndex);
			priceListStr.removeAt(currentIndex);
			volumeListStr.removeAt(currentIndex);
			sizeListStr.removeAt(currentIndex);
			endRemoveRows();
			somethingChanged=true;
		}
		return;
	}
	if(matchListRang&&priceList.at(currentIndex)==price)
	{//Update
		if(volumeList.at(currentIndex)==volume)return;
		directionList[currentIndex]=volumeList.at(currentIndex)<volume?1:-1;
		volumeList[currentIndex]=volume;
		sizeList[currentIndex]=0.0;
		priceListStr[currentIndex]=item.priceStr;
		volumeListStr[currentIndex]=item.volumeStr;
		sizeListStr[currentIndex]="0.0";
		somethingChanged=true;
		emit dataChanged(index(currentIndex+grouped,0),index(currentIndex+grouped,columnsCount-1));
	}
	else
	{//Insert
		beginInsertRows(QModelIndex(), currentIndex+grouped, currentIndex+grouped);
		priceList.insert(currentIndex,price);
		volumeList.insert(currentIndex,volume);
		sizeList.insert(currentIndex,volume);
		directionList.insert(currentIndex,0);
		priceListStr.insert(currentIndex,item.priceStr);
		volumeListStr.insert(currentIndex,item.volumeStr);
		sizeListStr.insert(currentIndex,item.volumeStr);
		endInsertRows();
		somethingChanged=true;
	}
}

double DepthModel::rowPrice(int row)
{
	if(grouped&&row<2)
	{
		if(row==0)return groupedPrice;
		return 0.0;
	}
	row-=grouped;
	if(!originalIsAsk)row=priceList.count()-row-1;
	if(row<0||row>=priceList.count())return 0.0;
	return priceList.at(row);
}

double DepthModel::rowVolume(int row)
{
	if(grouped&&row<2)
	{
		if(row==0)return groupedVolume;
		return 0.0;
	}
	row-=grouped;
	if(!originalIsAsk)row=priceList.count()-row-1;
	if(row<0||row>=priceList.count())return 0.0;
	return volumeList.at(row);
}

double DepthModel::rowSize(int row)
{
	if(grouped&&row<2)return 0.0;
	row-=grouped;
	if(!originalIsAsk)row=priceList.count()-row-1;
	if(row<0||row>=priceList.count())return 0.0;
	return sizeList.at(row);
}

