#include <iostream>
#include <QPixmap>
#include <QBrush>
#include <QImage>
#include <QDebug>
#include <QTimeLine>
#include <QColor>
#include "StView.h"

using namespace std;

StView::StView(QWidget * parent)
 : QGraphicsView(parent)
{
    //設定視窗
    setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

    //設定moveTimer
    moveTimer = new QTimer();
    moveTimer->setTimerType (Qt::PreciseTimer);
    moveTimer->start (25);

    //設定scene
    stimulateScene = new StimulateScene();

    //設定PacManMachine
    pacManMachine = new PacManMachine();
    connect (pacManMachine , SIGNAL(setPacMan(PacMan * )) , this , SLOT(addPacMan(PacMan *)) );
    pacManMachine->readXml ();
    pacManMachine->readCsv ();
    cout<<"pacManList有"<<pacManMachine->pacManList->size ()<<"隻pacMan"<<endl;
    pacManMachine->sortPacMans ();
    for(int i=0; i<6; ++i)
    {
        cout<<"第"<<i+1<<"隻pacMan的startSec為"<<pacManMachine->pacManList->at(i)->startSec<<endl;
    }

    //設定背景圖片
    backgroundPath = "./Data/" + pacManMachine->imageFile;
    qDebug()<<"路徑為"<<backgroundPath;

    //設定scene
    stimulateScene->setSceneRect ( 0 , 0 , QImage(backgroundPath).width () , QImage(backgroundPath).height ());
    qDebug()<<"scene's width and height"<<stimulateScene->width ()<<","<<stimulateScene->height ();
    stimulateScene->setBackgroundBrush (QBrush(QImage(backgroundPath)));
    setScene (stimulateScene);


    //設定View
    setFixedSize (QImage(backgroundPath).width ()+2 , QImage(backgroundPath).height ()+2 );
    connect (this , SIGNAL(setSize(int,int)) , parent , SLOT(setSize(int,int)) );
    emit setSize ( QImage(backgroundPath).width ()+2 , QImage(backgroundPath).height ()+2 );
    cout<<"emited"<<endl;

    //設定title
    parent->setWindowTitle (pacManMachine->title);

    //設定clock
    clock = new Clock();
    clock->setPos (QImage(backgroundPath).width() * pacManMachine->clockPosX , QImage(backgroundPath).height() * pacManMachine->clockPosY);
    clockTimer = new QTimer();
    clockTimer->setTimerType (Qt::PreciseTimer);
    connect(clockTimer , SIGNAL(timeout()) , clock , SLOT(addSec()) );
    connect(clock , SIGNAL(reload()) , this , SLOT(reload()) );
    //clock->reloadCycle = pacManMachine->reloadCycle;

    //開始模擬
    pacManMachine->spawnPacMans ();
    clockTimer->start(1000);
    stimulateScene->addItem (clock);

    //something to indicate the center
    centerPoint = new QGraphicsRectItem( -5 , -5 , 10 , 10);
    centerPoint->setBrush (QBrush(QColor(Qt::yellow)));
    scene()->addItem (centerPoint);

    //set mouse tracking to false
    isTracking = false;

    //set center
    //center_x = mapToScene(viewport()->rect()).boundingRect().center().x();
    //center_y = mapToScene(viewport()->rect()).boundingRect().center().y();
    //qDebug()<<"The center now is "<<center_x<<","<<center_y<<endl;
}

void StView::wheelEvent(QWheelEvent *event)
{
    if(event->delta()>0)
    {
        //cout<<"放大"<<endl;
        currentScale+=event->delta();
    }
    else
    {
        //cout<<"縮小"<<endl;
        currentScale+=event->delta();
    }

    if(currentScale<scaleMax)
    {
        //cout<<"拉回來"<<endl;
        currentScale-=event->delta();
        return;
    }

    double factor = event->delta()/100.0;
    if(event->delta()>0)
    {
        factor = factor;
    }
    else
    {
        factor = -1/factor;
    }

    scale(factor , factor);

    //cout<<"currentScale為"<<currentScale<<endl;
}

void StView::keyPressEvent(QKeyEvent *event)
{
    if(event->key () == Qt::Key_A)
    {
        centerOn (
                    mapToScene(viewport()->rect()).boundingRect().center().x () -10
                  , mapToScene(viewport()->rect()).boundingRect().center().y ()
                    );
        qDebug()<<"The Center Is Now At"
               <<mapToScene(viewport()->rect()).boundingRect().center().x ()
              <<" , "<<mapToScene(viewport()->rect()).boundingRect().center().y ()<<"\n";
    }
    else if (event->key () == Qt::Key_D)
    {
        centerOn (
                    mapToScene(viewport()->rect()).boundingRect().center().x () +10
                  , mapToScene(viewport()->rect()).boundingRect().center().y ()
                    );
        qDebug()<<"The Center Is Now At"
               <<mapToScene(viewport()->rect()).boundingRect().center().x ()
              <<" , "<<mapToScene(viewport()->rect()).boundingRect().center().y ()<<"\n";
    }
}


//PRESS MOUSE
void StView::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        saveWindowX = event->pos ().x ();
        saveWindowY = event->pos ().y ();

        QPoint windowPressPoint( saveWindowX ,  saveWindowY );
        saveDragCenterX = mapToScene (windowPressPoint).x();
        saveDragCenterY = mapToScene (windowPressPoint).y();

        qDebug()<<"You Clicked Window At"<< event->pos ().x ()<<" , "<<event->pos ().y ();
        qDebug()<<"You Clicked Scene At"<< saveDragCenterX<<" , "<<saveDragCenterY;

        isTracking = true;

        //test
        qDebug()<<"Set The Center At"<< saveDragCenterX<<" , "<<saveDragCenterY;
        centerOn (saveDragCenterX , saveDragCenterY);
        qDebug()<<"The Center Is Now At"
              <<mapToScene(viewport()->rect()).boundingRect().center().x ()
              <<" , "<<mapToScene(viewport()->rect()).boundingRect().center().y ()<<"\n";
        //centerPoint->setPos(mapToScene(viewport()->rect()).boundingRect().center().x () , mapToScene(viewport()->rect()).boundingRect().center().y ());
    }
}

void StView::mouseReleaseEvent(QMouseEvent *event)
{
    isTracking = false;
}

void StView::mouseMoveEvent(QMouseEvent *event)
{
    if(isTracking == true)
    {
        int dragX = event->pos ().x () - saveWindowX;
        int dragY = event->pos ().y () - saveWindowY;
        qDebug()<<"You Dragged The Window For"<<dragX<<" , "<<dragY;

        QRect fuck = viewport()->rect();
        fuck.setX(fuck.x ()+ dragX);
        fuck.setY(fuck.y ()+ dragY);
        //fuck.setRect (mapToScene (fuck));


        centerOn (mapToScene(fuck).boundingRect().center().x () ,mapToScene(fuck).boundingRect().center().y () );


        saveWindowX = event->pos ().x ();
        saveWindowY = event->pos ().y ();

        //QPoint windowPressPoint( saveWindowX ,  saveWindowY );
        //saveDragCenterX = mapToScene (windowPressPoint).x();
        //saveDragCenterY = mapToScene (windowPressPoint).y();



        centerOn (saveDragCenterX , saveDragCenterY);
        saveWindowX = event->pos ().x ();
        saveWindowY = event->pos ().y ();
        QPoint windowPressPoint( saveWindowX ,  saveWindowY );
        saveDragCenterX = mapToScene (windowPressPoint).x();
        saveDragCenterY = mapToScene (windowPressPoint).y();
    }
}

void StView::addPacMan(PacMan * pacMan)
{
    int x = (double)QImage(backgroundPath).width() * pacMan->startX;
    int y = (double)QImage(backgroundPath).height() * pacMan->startY;
    pacMan->setPos( x , y );




    //以第一號來說
    //x座標要在，(6-2)4秒之內，走圖片寬度*(0.9-0.05)
    //然後timer每次跑是跑0.1秒

    int runSec = pacMan->liveSec;
    double runLengthX = (double)QImage(backgroundPath).width() * (pacMan->endX - pacMan->startX);
    double runLengthY = (double)QImage(backgroundPath).height () * (pacMan->endY - pacMan->startY);

    double xScale = ((double)runLengthX / runSec ) / (1000/moveTimer->interval ());
    double yScale = ((double)runLengthY / runSec ) / (1000/moveTimer->interval ());

    pacMan->setXYScale (xScale , yScale);

    stimulateScene->addItem(pacMan);
    //QTimer::singleShot ( pacMan->liveSec *1000 , pacMan , SLOT(destroySelf()) );
    QTimer * timer = new QTimer();
    timer->setTimerType (Qt::PreciseTimer);
    connect(timer , SIGNAL(timeout()) , pacMan , SLOT(destroySelf()) );
    timer->start (pacMan->liveSec *1000);
    connect(moveTimer , SIGNAL(timeout()) , pacMan , SLOT(move()) );
}

void StView::reload()
{
    pacManMachine->readCsv ();
    pacManMachine->sortPacMans ();
    pacManMachine->spawnPacMans ();
}
