tables.to.bytes <- function(bits, rules, numtables) {
  N <- 8 * ceiling(rules / 8)
  nt <- numtables
  even <- (nt - (bits %% nt))*(2^(bits %/% nt))*N
  odd <- (bits %% nt)*(2^((bits %/% nt) + 1))*N
  return((even + odd) / 8)
}

loadin.12K <- function(pattern){
  files <- lapply(c(list.files(pattern=pattern)), read.csv, head=T)
  swissed <- lapply(files,
                    function(x){ #trim some fat
                      x[c(  1: 50,seq( 51,101,15),
                          102:151,seq(152,202,15),
                          203:252,seq(253,303,15),
                          304:353,seq(354,404,15)
                          ),]
                    }
                    )
  return(merge.all(swissed))
}
comma <- function(x){
  return (formatC(x,format="d", big.mark=","))
}
dot <- function(x){
  return (formatC(x,format="d", big.mark="."))
}

real.pps.vs.tables <- function(pattern, bits, dir="./pdfs"){
  if(bits == 12000){
    total <- loadin.12K(pattern)
  }else{
    total <- loadin(pattern)
  }
  real.pps.ed <- add.real.pps(total)
  rsp <- splitup.by(real.pps.ed, "number.of.rules")
  cexval <- 1.5
  pps.label <- "Throughput (PPS)"
  mbps.label <- "Throughput (Mbps)"
  tab.label <- "Number of Tables"
  mem.label <- "Memory Required (MB)"
  for(i in 1:length(rsp)){
    numrules <- rsp[[i]]$number.of.rules[1]
    print(rsp[[i]]$number.of.rules[1])
    filename <- paste("Tables_vs_PPS_",dot(bits),"bits_",
                      dot(numrules),"rules.pdf",sep="")
    titletext <- paste("Throughputs for", comma(bits),
                       "Bits Classified, with", comma(numrules),"Rules")
    pdf(file=paste(dir,filename,sep="/"), title=titletext,
        width=11, height=8.5, paper="USr")
    par(mar=c(3,4,5,4), cex=cexval, bty="o")
    options(scipen=10)
    plot(x = rsp[[i]]$number.of.tables, y = rsp[[i]]$real.pps,
         xlab = "", ylab = pps.label, xaxt="n",
         yaxt="n", tck=0, log="")
    title(main = titletext, line = 4)
    tab.ticks <- pretty(rsp[[i]]$number.of.tables, 50)
    pps.ticks <- pretty(rsp[[i]]$real.pps, 10)
    axis(1, tab.ticks, labels = tab.ticks)
    mtext(1, text = tab.label, line = 2, cex=cexval)
    axis(2, pps.ticks, labels = paste(pps.ticks / 1000,"K",sep=""), las = 1)
    axis(4, at = pps.ticks, labels = pps.ticks * 0.012, las=1)
    mtext(4, text = mbps.label, line=3, cex=cexval)
    mem.ticks <- pretty(tab.ticks, 20)
    mem.labels <- tables.to.bytes(bits, numrules, mem.ticks)/ 1000000
    options(scipen=0)
    axis(3, at = mem.ticks, labels = mem.labels)
    mtext(3, text = mem.label, line = 2, cex=cexval)
    dev.off()
  }
}

#specified
real.pps.vs.tables.104 <- function(){
  real.pps.vs.tables("alltest104bits_[1-3].csv",104)
}
real.pps.vs.tables.320 <- function(){
  real.pps.vs.tables("alltest320bits_[1-3].csv",320)
}
real.pps.vs.tables.12K <- function(){
  real.pps.vs.tables("maxtest_[1-3].csv",12000)
}


#returns a data frame consisting of the merged data from all files patching the
#pattern
loadin <- function(pattern){
  files <- lapply(c(list.files(pattern = pattern)), read.csv, head=T)
  return(merge.all(files))
}

#takes a data frame and splits it along the number.of.rules factor and returns a
#list of data frames
splitup.by <- function(x, field) {
  xlevels <- unique(x[field])
  y <- list()
  for( i in 1:length(xlevels[[1]])){
    y[[i]] <- subset(x, x[field] == xlevels[[1]][[i]], all=T)
  }
  return(y)
}

merge.all <- function(x) {
  merged <- x[[1]]
  if(length(x) > 1){
    for(i in 2:length(x)){
      merged <- merge(merged, x[[i]], all=T)
    }
  }
  return(merged)
}

maxmin.throughput.graph <- function(dir = "./pdfs"){
  # plots the maxes and mins of different test runs
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 #, loadin("alltest64bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin("maxtest_[1-3].csv")
                 )
  #split each by number of rules
  rulesplit <- lapply(totals, splitup.by, field="number.of.rules")
  #now get min and max for each rule/bit element
  minmaxed <- list()
  for( i in 1:length(rulesplit)){
    minmaxed[[i]] <- lapply(rulesplit[[i]], minmax, field="memory.used")
  }
  #remerge the results
  rmgd <- lapply(minmaxed, merge.all)
  #draw the plot
  pdf(paste(dir,"classifier_throughputs.pdf",sep="/"),
      width=11, height=8.5, paper="USr")
  options(scipen = 10)
  par(cex=1.5, bty="u", mar=c(5,4,4,5), tcl=-0.3)
  plot(x = rmgd[[1]]$number.of.rules, y = 500000 / rmgd[[1]]$real.process.time,
       xlab = "Number of Rules", ylab = "PPS",
       ylim =  c(100,10000000), xlim = c(50,1500000),
       main = "Classifier Throughputs", log="xy",
       pch = 1, xaxt="n", yaxt="n", tck = 0
       )
  ppsticks <- c(100, 1000, 10000, 100000, 1000000, 10000000)
  mbpsticks <- c(1,10,100,1000,10000,100000)/0.012
  xticks <- c(100, 1000, 10000, 100000, 1000000)
  mbpslabels <- c("1 Mbps","10 Mbps","100 Mbps","1 Gbps","10 Gbps", "100 Gbps")
  ppslabels <- c("100", "1K", "10K", "100K", "1M", "10M")
  xlabels <- c("100", "1K", "10K", "100K", "1M")
  axis(4, at = mbpsticks, labels = mbpslabels,las=1)
  axis(2, at=ppsticks, labels = ppslabels)
  axis(1, at=xticks, labels = xlabels)
  for(i in 2:length(rmgd)){
    if(rmgd[[i]][1,]$packet.size.in.bits == 12000){
      points(x = rmgd[[i]]$number.of.rules,
             y = 10000 / rmgd[[i]]$real.process.time, pch = i+1)
    }else{
      points(x = rmgd[[i]]$number.of.rules,
             y = 500000 / rmgd[[i]]$real.process.time, pch = i+1)
    }
  }
  legend(x="bottomleft",legend=c(32,104,320,12000),
         pch=c(1,3,4,5), title="Bits Classified")
  dev.off()
}

#returns a data set with only the minimum and maximum of the given field
minmax <- function(x, field) {
  x.min <- subset(x, x[field] == min(x[field]))[1,]
  x.max <- subset(x, x[field] == max(x[field]))[1,]
  x.final <- merge(x.min, x.max, all=T)
}

build.time.vs.rules <- function (dir = "./pdfs") {
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 #, loadin("alltest64bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin("maxtest_[1-3].csv")
                )
  #split each data set up by the number of rules
  rulesplit <- lapply(totals, splitup.by, field="number.of.rules")
  #then throw away all but the min and max
  minmaxed <- list()
  for(i in 1:length(rulesplit)){
    minmaxed[[i]] <- lapply(rulesplit[[i]], minmax, field="table.build.time")
  }
  # then re-merge the results
  rmgd <- lapply(minmaxed, merge.all)
  
  pdf(paste(dir,"build_time_vs_rules.pdf",sep="/"),
      height=8.5, width=11, paper="USr")
  options(scipen = 10)
  par(cex = 1.5 , bty="l")
  plot(x = rmgd[[1]]$number.of.rules, y = rmgd[[1]]$table.build.time, log="xy",
       xlim = c(100,1000000), ylim = c(0.001,2000), 
       xlab = "Number of Rules", ylab = "Table-build Time (s)",
       main = "Table-build Times", pch = 1, axes=T, xaxt="n", yaxt="n", tck=0.0)
  yticks <- c(0.001, 0.01, 0.1, 1, 10, 100, 1000)
  xticks <- c(100, 1000, 10000, 100000, 1000000)
  xlabels <- c("100", "1K", "10K", "100K", "1M")
  axis(2, at=yticks, labels = prettyNum(yticks, drop0trailing = T))
  axis(1, at=xticks, labels = xlabels)
  for( i in 2:length(rmgd)){
    points(x = rmgd[[i]]$number.of.rules, y = rmgd[[i]]$table.build.time, pch = i+1)
  }
  legend(x="bottomright",legend=c(32,104,320,12000),
         pch=c(1,3,4,5), title="Bits Classified")
  dev.off()
}

# adds a field to the data set x that contains real pps
add.real.pps <- function (x) {
  if(x$packet.size.in.bits[1] == 12000){
    x$real.pps <- 10000 / x$real.process.time
  }else{
    x$real.pps <- 500000 / x$real.process.time
  }
  return(x)
}

series.of.4.graph <- function(numrules,
                              xlimits,
                              ylimits,
                              xlabels,
                              ppslabel,
                              mbps.label = "Throughput (Gbps)",
                              ppsdiv = 1000000,
                              bpsmul = 0.000012,
                              xlas = 1,
                              mar = c(3,4,4,4),
                              xaxticks = F,
                              wherelegend = "topright",
                              xaxis.line = 2,
                              pps.line = 2,
                              bps.line = 2,
                              dir="./pdfs"){
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin.12K("maxtest_[1-3].csv")
                 )
  titletext <- paste("Throughputs for", comma(numrules),"Rules")
  pps.included <- lapply(totals, add.real.pps)
  only <- lapply(pps.included, function(x) subset(x, x$number.of.rules == numrules))
  cexsize <- 1.5
  pdf(paste(dir,paste("4series_",dot(numrules),"_rules.pdf",sep=""),sep="/"),
      height=8.5, width=11, paper="USr", title = titletext)
  options(scipen = 10)# only shorten to scientific notation if difference is >
                      # 10 characters
  par(cex = cexsize , bty="u", mar = mar, ann = F)
  plot(x = only[[1]]$memory.used, y = only[[1]]$real.pps, log="x",
       xlim = xlimits, ylim = ylimits,
       pch = 1,
       xaxt="n",yaxt="n", tck=0.0
       )
  xlab <- "Memory Used"
  pps.ticks <- pretty(ylimits, 10)
  if(xaxticks[1] == F){
    axis(1, at = axTicks(1,log=T), labels = xlabels, las = xlas)
  }else{
    axis(1, at = xaxticks, labels = xlabels, las = xlas)
  }
  title(main = titletext)
  mtext(1, text = xlab, line = xaxis.line , cex = cexsize)
  axis(2, at = pps.ticks, labels = pps.ticks / ppsdiv, las=1)
  mtext(2, text = ppslabel, line = pps.line, cex = cexsize)
  axis(4, at = pps.ticks, labels = pps.ticks * bpsmul, las = 1)
  mtext(4, text = mbps.label, line = bps.line, cex = cexsize)
  for( i in 2:length(only)){
    points(x = only[[i]]$memory.used, y = only[[i]]$real.pps, pch = i+1)
  }
  legend(x=wherelegend, legend=c(32,104,320,12000),
         pch=c(1,3,4,5), title="Bits Classified")
  dev.off()
}

#specialized for 1K
series.of.4.graph.1K <- function(){
  numrules <- 1000
  xlimits <- c(10000, 2000000000)
  ylimits <- c(1000,  1750000)
  ppslabel <- "Throughput (Million PPS)"
  xlabels <- c("10KB", "100KB", "1MB", "10MB", "100MB", "1GB")
  series.of.4.graph(numrules,xlimits,ylimits,xlabels,ppslabel)
}

series.of.4.graph.10K <- function(){
  numrules <- 10000
  xlimits <- c(50000, 2000000000)
  ylimits <- c(1, 230000)
  ppsdiv <- 1000
  ppslabel <- "Throughput (Thousand PPS)"
  xlabels <- c("100KB", "1MB", "10MB", "100MB", "1GB")
  wherelegend <- "bottomleft"
  series.of.4.graph(numrules = numrules,
                    xlimits = xlimits,
                    ylimits = ylimits,
                    xlabels = xlabels,
                    ppslabel = ppslabel,
                    ppsdiv = ppsdiv,
                    wherelegend = wherelegend)
}
thou <- function(x){
  return(x*1000)
}

mil <- function(x){
  return (x*1000000)
}
bil <- function(x){
  return (x*1000000000)
}

series.of.4.graph.100K <- function(){
  numrules <- thou(100)
  xlimits <- c(mil(1), bil(2))
  ylimits <- c(1, thou(30))
  ppsdiv <- 1000
  bpsmul <- 0.012
  ppslabel <- "Throughput (Thousand PPS)"
  mbps.label <- "Throughput (Mbps)"
  xaxticks <- c(mil(1),mil(3),mil(10),mil(30),mil(100),mil(300),bil(1),bil(2))
  xlabels <- c("1MB","3MB","10MB","30MB","100MB","300MB","1GB","2GB")
  wherelegend <- "bottomleft"
  xlas <- 1
  xaxis.line <- 2
  pps.line <- 2.5
  bps.line <- 2.5
  mar <- c(3,3.5,3,3.5)
  series.of.4.graph(numrules = numrules,
                    xlimits = xlimits,
                    ylimits = ylimits,
                    xlabels = xlabels,
                    ppslabel = ppslabel,
                    mbps.label = mbps.label,
                    ppsdiv = ppsdiv,
                    bpsmul = bpsmul,
                    wherelegend = wherelegend,
                    xlas = xlas,
                    xaxis.line = xaxis.line,
                    xaxticks = xaxticks,
                    bps.line = bps.line,
                    pps.line = pps.line,
                    mar = mar)
}
